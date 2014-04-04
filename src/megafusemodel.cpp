#include "mega.h"

#include "megacrypto.h"
#include "megaclient.h"
#include "megafusemodel.h"

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>









std::set<std::string> MegaFuseModel::readdir(const char *path)
{
	std::set<std::string> names;

	auto p = splitPath(path);
	std::string p2 = p.first;
	for(auto it = cacheManager.cbegin(); it != cacheManager.cend(); ++it) {
		auto namePair = splitPath(it->first);
		if(namePair.first == std::string(path) && it->second.status != file_cache_row::INVALID)
			names.insert(namePair.second.c_str());
	}


	return names;
}



int MegaFuseModel::rename(const char * src, const char *dst)
{
	printf("--------requested rename from %s to %s\n",src,dst);
	std::lock_guard<std::mutex>lock(api_mutex);


	if(cacheManager.find(src) == cacheManager.end() )
		return -ENOENT;
	std::swap(cacheManager[dst],cacheManager[src]);
	std::swap(cacheManager[dst].n_clients,cacheManager[src].n_clients);
	cacheManager[src].status = file_cache_row::INVALID;
	if(cacheManager[src].n_clients <= 0)
		eraseCacheRow(cacheManager.find(src));
	return 0;



}

MegaFuseModel::MegaFuseModel(EventsHandler &eh,std::mutex &em):eh(eh),engine_mutex(em),cacheManager(),file_cache(cacheManager.file_cache)
{

}

int MegaFuseModel::getAttr(const char *path, struct stat *stbuf)
{
	std::lock_guard<std::mutex>lock(api_mutex);

	printf("-------------getattr requested, %s\n",path);

	Node *n = nodeByPath(path);

	for(auto it = cacheManager.cbegin(); it != cacheManager.cend(); ++it)
		if(it->first == std::string(path)) {
			stbuf->st_mode = S_IFREG | 0555;
			stbuf->st_nlink = 1;

			struct stat st;
			stbuf->st_size = it->second.size;
			stbuf->st_mtime = it->second.last_modified;
			if (stat(it->second.localname.c_str(), &st) == 0) {
				//stbuf->st_size =st.st_size;
				stbuf->st_mtime = st.st_mtime;
				//printf("recuperate dalla cache : %d\n",stbuf->st_size);
			}

			return 0;

		}
	printf("not found in cache\n");

	return -ENOENT;


}

std::pair<std::string,std::string> MegaFuseModel::splitPath(std::string path)
{
	int pos = path.find_last_of('/');
	std::string basename = path.substr(0,pos);
	std::string filename = path.substr(pos+1);
	if(basename == "")
		basename = "/";
	return std::pair<std::string,std::string> (basename,filename);
}

//warning, no mutex
Node* MegaFuseModel::nodeByPath(std::string path)
{
	printf("searching node by path %s\n",path.c_str());
	if(engine_mutex.try_lock()) {
		printf("errore mutex in nodebypath\n");
		abort();
	}
	if (path == "/") {
		Node*n = client->nodebyhandle(client->rootnodes[0]);
		n->type = ROOTNODE;
		return n;
	}
	if(path[0] == '/')
		path = path.substr(1);
	Node *n = client->nodebyhandle(client->rootnodes[0]);

	int pos;
	while ((pos = path.find_first_of('/')) > 0) {


		n = childNodeByName(n,path.substr(0,pos));
		if(!n)
			return NULL;
		path = path.substr(pos+1);
	}
	n = childNodeByName(n,path);
	if(!n)
		return NULL;
	printf("nodo trovato in MEGA: %s\n",path.c_str());
	return n;
}

Node* MegaFuseModel::childNodeByName(Node *p,std::string name)
{
	if(engine_mutex.try_lock()) {
		abort();
	}
	for (node_list::iterator it = p->children.begin(); it != p->children.end(); it++) {
		if (!strcmp(name.c_str(),(*it)->displayname())) {
			return *it;
		}
	}
	printf("file %s not found in MEGA\n",name.c_str());
	return NULL;
}



std::unordered_map <std::string,file_cache_row>::iterator MegaFuseModel::eraseCacheRow(std::unordered_map <std::string,file_cache_row>::iterator it)
{
	if(it->second.n_clients != 0)
		return ++it;
	
	it = file_cache.erase(it);
	return it;
}

void MegaFuseModel::check_cache()
{
	if(file_cache.size() < 2)
		return;
	std::lock_guard<std::mutex>lock(engine_mutex);
	for(auto it = cacheManager.begin(); it!= cacheManager.end(); ++it)
		if(it->second.n_clients==0 && it->second.status ==file_cache_row::AVAILABLE) {
			printf("rimuovo il file %s dalla cache\n",it->first.c_str());
			if(it->second.status ==file_cache_row::DOWNLOADING) //UPLOADING FILES IGNORED
				client->tclose(it->second.td);
			eraseCacheRow(it);
		}
	printf("check cache \n");
}

void createthumbnail(const char* filename, unsigned size, string* result);



int MegaFuseModel::release(const char *path, struct fuse_file_info *fi)
{
	int ret = 0;
	std::lock_guard<std::mutex>engine_lock(api_mutex);
	auto it = cacheManager.begin();

	std::unique_lock<std::mutex> lock(engine_mutex);
	it = cacheManager.find(std::string(path));
	it->second.n_clients--;

	if(!it->second.n_clients && it->second.modified) {
		Node *oldNode = nodeByPath(it->first);

		auto target = splitPath(it->first);
		Node *n = nodeByPath(target.first);
		//if(target.second[0] == '.')
		//return 0;
		//client->putq.push_back(new MegaFuseFilePut(it->second.localname.c_str(),n->nodehandle,"",target.second.c_str(),it->first));
		int td = client->topen(it->second.localname.c_str(),-1,2);
		if (td < 0)
			return -EAGAIN;
		it->second.td = td;
		it->second.status = file_cache_row::UPLOADING;

		std::string thumbnail;
		createthumbnail(it->second.localname.c_str(),120,&thumbnail);

		if (thumbnail.size()) {
			cout << "Image detected and thumbnail extracted, size " << thumbnail.size() << " bytes" << endl;
			handle uh = client->uploadhandle(td);
			client->putfa(&client->ft[td].key,uh,THUMBNAIL120X120,(const byte*)thumbnail.data(),thumbnail.size());
		}




		if(it->second.status ==file_cache_row::UPLOADING) {
			EventsListener el(eh,EventsHandler::UPLOAD_COMPLETE);
			EventsListener el2(eh,EventsHandler::NODE_UPDATED);
			lock.unlock();
			auto l_ress = el.waitEvent();

			auto l_res = el2.waitEvent();
			if(oldNode) {
				lock.lock();
				client->unlink(oldNode);
				lock.unlock();
			}

		}



	}


	printf("release chiamato: il file %s e' ora utilizzato da %d utenti\n",it->first.c_str(),it->second.n_clients);


	//check_cache();
	return ret;
}

/*bool MegaFuseModel::chunksAvailable(std::string filename,int startOffset,int size)
{
	auto it = cacheManager.find(std::string(filename));
	if(it == cacheManager.end())
		return false;
	int startChunk = numChunks(ChunkedHash::chunkfloor(startOffset));//startOffset/CHUNKSIZE;
	int endChunk = numChunks(startOffset+size);
	bool available = true;
	for(int i = startChunk; i < endChunk && i < (int) it->second.availableChunks.size(); i++) {
		available = available && it->second.availableChunks[i];
	}
	//printf("chunksavailable da %d a %d, blocchi da %d a %d escluso. riferimento: %d\n",startOffset,startOffset+size,startChunk,endChunk,ChunkedHash::chunkfloor(startOffset));
	return available;
}*/


int MegaFuseModel::enqueueDownload(std::string remotename,int startOffset=0)
{
	Node *n;
	
		std::unique_lock<std::mutex>lock(engine_mutex);

		n = nodeByPath(remotename.c_str());
		if(!n)
			return -ENOENT;


		{
			auto it = cacheManager.find(remotename);
			if(it != cacheManager.end() && it->second.td >= 0) {

				if(it->second.status == file_cache_row::DOWNLOADING) {
					client->tclose(it->second.td);
					it->second.td = -1;
				} else {
					//the local copy is better and the file is being uploaded
					return 0;
				}
			}
		}


		EventsListener el(eh,EventsHandler::TOPEN_RESULT);


		int td = client->topen(n->nodehandle, NULL, startOffset,-1, 1);

		if(td < 0)
			return -EIO;
		cacheManager[remotename].td = td;
		if(cacheManager[remotename].status != file_cache_row::DOWNLOADING && cacheManager[remotename].status != file_cache_row::DOWNLOAD_PAUSED)
			cacheManager[remotename].status = file_cache_row::INVALID;
	lock.unlock();
	cacheManager[remotename].last_modified = n->mtime;
	cacheManager[remotename].size = n->size;
	cacheManager[remotename].startOffset = startOffset;
	cacheManager[remotename].available_bytes = 0;

	printf("opend: aspetto il risultato per il file %s\n",remotename.c_str());
	
			
	auto l = el.waitEvent();
	return l.result;
		
	
}


int MegaFuseModel::open(const char *p, struct fuse_file_info *fi)
{
	auto sPath = splitPath(p);
	//if(sPath.second[0] == '.')
	//return -EINVAL;
	/*if((fi->flags & O_WRONLY) || ((fi->flags & (O_RDWR | O_TRUNC)) == (O_RDWR | O_TRUNC))) {
		return create(p,755,fi);


	}*/


	printf("flags:%X\n",fi->flags);
	std::lock_guard<std::mutex>lock(api_mutex);
	std::string path(p);

	//Workaround for kde bug in debian 7
	if(sPath.second.find(".directory") == 0 && (fi->flags & O_EXCL))
		return -EPERM;

	std::unique_lock <std::mutex> engine(engine_mutex);

	Node *n = nodeByPath(p);
	auto it = cacheManager.find(path);
	if(it != cacheManager.end()  && it->second.status == file_cache_row::DOWNLOAD_PAUSED) {
		printf("resuming paused download11\n");
	}
	/*files with 0 clients are OK, it's a cache*/
	bool oldCache = it != cacheManager.end() && n && it->second.last_modified < n->mtime;

	if(oldCache && it->second.n_clients > 0)
		return -EAGAIN;
	if(oldCache && it->second.n_clients <= 0) {
		eraseCacheRow(it);
		it = cacheManager.end();
	}

	bool fileExists = n || it != cacheManager.end();

	if(!fileExists && !(fi->flags & O_CREAT))
		return -ENOENT;
	if(fileExists && (fi->flags & O_CREAT) && (fi->flags & O_EXCL))
		return -EEXIST;

	if(fi->flags & O_TRUNC || (!fileExists && (fi->flags & O_CREAT)) ) {
		if(cacheManager[p].status == file_cache_row::DOWNLOADING) {
			client->tclose(cacheManager[p].td);
			cacheManager[p].td = -1;
		}
		cacheManager[p].status = file_cache_row::AVAILABLE;
		cacheManager[p].size=0;
		cacheManager[p].modified=true;
		cacheManager[p].n_clients++;

		truncate(cacheManager[p].localname.c_str(), 0);
		return 0;
	}

	if(it != cacheManager.end() && it->second.status != file_cache_row::INVALID && it->second.status != file_cache_row::DOWNLOAD_PAUSED) {
		it->second.n_clients++;
		return 0;
	}
	if(it != cacheManager.end()  && it->second.status == file_cache_row::DOWNLOAD_PAUSED) {
		printf("resuming paused download\n");
	} else {
		cacheManager[p].status = file_cache_row::INVALID;
		it = cacheManager.find(p);

	}
	engine.unlock();



	int res = enqueueDownload(p,0);

	if(res < 0)
		return -EIO;

	engine.lock();
	it->second.n_clients++;
	it->second.handle = n->nodehandle;
	return 0;


}


int MegaFuseModel::blockOffset(int pos)
{
	m_off_t end = 0;

	for(int i=1; i<=8; i++) {
		if(i> pos) return end;
		end +=i*ChunkedHash::SEGSIZE;
	}
	return (pos-8)*8*ChunkedHash::SEGSIZE + end;
}


int MegaFuseModel::numChunks(m_off_t pos)
{
	m_off_t end = 0;
	if(pos == 0)
		return 0;
	for(int i=1; i<=8; i++) {
		end +=i*ChunkedHash::SEGSIZE;
		if(end >= pos) return i;
	}
	return 8 + ceil(float(pos-end)/(8.0*ChunkedHash::SEGSIZE));


}

int MegaFuseModel::write(const char * path, const char *buf, size_t size, off_t offset, struct fuse_file_info * fi)
{
	std::lock_guard<std::mutex>lock(api_mutex);

	auto it = cacheManager.find(path);

	chmod(it->second.localname.c_str(),S_IWUSR|S_IRUSR);
	printf("write, file %s, apro cache: %s\n",it->first.c_str(),it->second.localname.c_str());
	int fd = ::open(it->second.localname.c_str(),O_WRONLY);
	if (fd < 0 )
		return fd;
	int s = pwrite(fd,buf,size,offset);
	close(fd);
	it->second.modified=true;
	int newsize = offset + size;
	if(it->second.size < newsize) {
		it->second.size = newsize;

		it->second.availableChunks.resize(numChunks(it->second.size),false);

	}
	return s;
}


int MegaFuseModel::read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("read requested, offset %ld, size %ld\n",offset,size);
	std::lock_guard<std::mutex>lock(api_mutex);



	std::unique_lock<std::mutex> engine(engine_mutex);

	printf("apro cache: %s\n",cacheManager[path].localname.c_str());
	auto it = cacheManager.find(path);
	if(it->second.status == file_cache_row::INVALID)
		return -EIO;
	bool canWait = ((offset+size) - (it->second.available_bytes)) < 1024*1024 && it->second.startOffset <= offset;




	if(!it->second.canRead(offset,size)) {
		if(!canWait) {
			printf("--------------read too slow, downloading the requested chunk\n");
			int startOffset = ChunkedHash::chunkfloor(offset);
			engine.unlock();
			int res = enqueueDownload(path,startOffset);
			if(res < 0)
				return res;
			engine.lock();
		}

		printf("mi metto in attesa di ricevere i dati necessari\n");
		EventsListener el(eh,EventsHandler::TRANSFER_UPDATE);

		while(!it->second.canRead(offset,size)) {
			engine.unlock();
			el.waitEvent();
			engine.lock();

		}

	}


	int fd = ::open(cacheManager[path].localname.c_str(),O_RDONLY);
	if (fd < 0 ) {
		printf("open fallita in read per il file %s\n",cacheManager[path].localname.c_str());
		//engine_mutex.unlock();
		return -EIO;
	}

	//int base = offset-(it->second.startOffset);
	printf("-----offset richiesto: %ld, offset della cache: %d,status %d,availablebytes %d\n",offset, it->second.startOffset,it->second.status,it->second.available_bytes);
	int s = pread(fd,buf,size,offset);
	close(fd);
	//engine_mutex.unlock();
	return s;
}


int MegaFuseModel::unlink(std::string filename)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	printf("-------------unlinking %s\n",filename.c_str());

	auto it = cacheManager.find(filename);

	if(it == cacheManager.end())
		return -ENOENT;



	if( it->second.td > -1)
		client->tclose(it->second.td);

	it->second.status = file_cache_row::INVALID;
	if( it->second.n_clients <= 0)
		eraseCacheRow(it);
	return 0;



}

MegaFuseModel::~MegaFuseModel()
{
	auto it = cacheManager.begin();
	while(it != cacheManager.end())
		it = eraseCacheRow(it);

}
