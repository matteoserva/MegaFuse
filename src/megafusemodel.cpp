#include "mega.h"

#include "megacrypto.h"
#include "megaclient.h"
#include "megafusemodel.h"

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>



file_cache_row::file_cache_row(): td(-1),status(INVALID),size(0),available_bytes(0),n_clients(0),startOffset(0),modified(false),handle(0)
{
	localname = tmpnam(NULL);
	tmpname = tmpnam(NULL);
	int fd = open(localname.c_str(),O_CREAT|O_WRONLY,S_IWUSR|S_IRUSR);
	close(fd);
	fd = open(tmpname.c_str(),O_CREAT|O_WRONLY,S_IWUSR|S_IRUSR);
	close(fd);
	printf("creato il file %s\n",localname.c_str());


}
file_cache_row::~file_cache_row()
{

}



bool MegaFuseModel::start()
{
	
	return true;
}

bool MegaFuseModel::login(std::string username, std::string password)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	{

		


	}
		return true;
}

std::set<std::string> MegaFuseModel::readdir(const char *path)
{
	std::set<std::string> names;

	auto p = splitPath(path);
	std::string p2 = p.first;
	for(auto it = file_cache.cbegin(); it != file_cache.cend(); ++it)
	{
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
	
	
	Node *n_src ;
	Node *n_dst ;
	auto path = splitPath(dst);
	
	
	//move first
	{
		std::unique_lock<std::mutex>lockE(engine_mutex);
		n_src = nodeByPath(src);
		n_dst = nodeByPath(dst);
		//now we handle caches.
		bool sourceCached = file_cache.find(src) != file_cache.end(); 
		bool destCached = file_cache.find(dst) != file_cache.end(); 
		
		if(sourceCached && file_cache[src].n_clients > 0)
			return -EBUSY;
		if(destCached && file_cache[dst].n_clients > 0)
			return -EBUSY;
		
		
		/*if(destCached)
			eraseCacheRow(file_cache.find(dst));
		*/
		
		
		if(sourceCached)
		{   //we have the source file in cache.
			//create [dst] if needed
			printf("---rename src found in cache \n");
			std::swap(file_cache[dst],file_cache[src]);
			eraseCacheRow(file_cache.find(src));
			
			
		}
		else if(destCached)
		{
			eraseCacheRow(file_cache.find(dst));
			
		}
		
		
		
		if(!n_src)
		{
			if(sourceCached)
				return 0;
			else
				return -ENOENT;
		}
		
		Node *dstFolder = nodeByPath(path.first);
		
		if(!dstFolder)
				return -EINVAL;
		
		if ( client->rename(n_src,dstFolder) != API_OK)
				return -3;
		
		n_src->attrs.map['n'] = path.second.c_str();
		if (client->setattr(n_src))
				return -4;
		/*{
		printf("waiting update\n");
		EventsListener el(eh,EventsHandler::NODE_UPDATED);
		lockE.unlock();
		auto l_res = el.waitEvent();
		lockE.lock();
		}*/

		
		
		//delete overwritten file
		if(n_dst && n_dst->type == FILENODE) 
				client->unlink(n_dst);
		else
			return 0;
	}
	{
		EventsListener el(eh,EventsHandler::UNLINK_RESULT);
		auto l_res = el.waitEvent();
	}
	return 0;



}

MegaFuseModel::MegaFuseModel(EventsHandler &eh,std::mutex &em):eh(eh),engine_mutex(em)
{
	
}

int MegaFuseModel::getAttr(const char *path, struct stat *stbuf)
{
	std::lock_guard<std::mutex>lock(api_mutex);

	printf("-------------getattr requested, %s\n",path);
	{
		std::lock_guard<std::mutex>lock2(engine_mutex);

		std::cout <<"PATH: "<<path<<std::endl;
		Node *n = nodeByPath(path);

			printf("looking in cache\n");
			for(auto it = file_cache.cbegin(); it != file_cache.cend(); ++it)
				if(it->first == std::string(path)) {
					stbuf->st_mode = S_IFREG | 0555;
					stbuf->st_nlink = 1;

					struct stat st;
					stbuf->st_size = it->second.size;
					stbuf->st_mtime = it->second.last_modified;
					if (stat(it->second.localname.c_str(), &st) == 0)
					{
						//stbuf->st_size =st.st_size;
						stbuf->st_mtime = st.st_mtime;
						//printf("recuperate dalla cache : %d\n",stbuf->st_size);
					}
					
					return 0;

				}
				printf("not found in cache\n");
	}
	return -ENOENT;
	

	return 0;

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



std::map <std::string,file_cache_row>::iterator MegaFuseModel::eraseCacheRow(std::map <std::string,file_cache_row>::iterator it)
{
	if(it->second.n_clients != 0)
		return ++it;
	::unlink(it->second.localname.c_str());
	::unlink(it->second.tmpname.c_str());
	it = file_cache.erase(it);
	return it;
}

void MegaFuseModel::check_cache()
{
	if(file_cache.size() < 2)
		return;
	std::lock_guard<std::mutex>lock(engine_mutex);
	for(auto it = file_cache.begin(); it!= file_cache.end(); ++it)
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
	std::lock_guard<std::mutex>lock(api_mutex);
	auto it = file_cache.begin();
	{
		std::unique_lock<std::mutex> lock(engine_mutex);
		it = file_cache.find(std::string(path));
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

			if (thumbnail.size())
			{
				cout << "Image detected and thumbnail extracted, size " << thumbnail.size() << " bytes" << endl;
				handle uh = client->uploadhandle(td);
				client->putfa(&client->ft[td].key,uh,THUMBNAIL120X120,(const byte*)thumbnail.data(),thumbnail.size());
			}
			
			
			
			
			if(it->second.status ==file_cache_row::UPLOADING)
			{
				EventsListener el(eh,EventsHandler::UPLOAD_COMPLETE);
				EventsListener el2(eh,EventsHandler::NODE_UPDATED);
				lock.unlock();
				auto l_ress = el.waitEvent();
				
				auto l_res = el2.waitEvent();
				if(oldNode)
				{
					lock.lock();
					client->unlink(oldNode);
					lock.unlock();
				}
			
			}
			
			
				
		}
	}
	
	printf("release chiamato: il file %s e' ora utilizzato da %d utenti\n",it->first.c_str(),it->second.n_clients);


	//check_cache();
	return ret;
}

bool MegaFuseModel::chunksAvailable(std::string filename,int startOffset,int size)
{
	auto it = file_cache.find(std::string(filename));
	if(it == file_cache.end())
		return false;
	int startChunk = startOffset/CHUNKSIZE;
	int endChunk = ceil(float(startOffset+size) / CHUNKSIZE);
	bool available = true;
	for(int i = startChunk;i < endChunk && i < (int) it->second.availableChunks.size();i++)
	{
		available = available && it->second.availableChunks[i];
	}
	return available;
}


int MegaFuseModel::enqueueDownload(std::string remotename,int startOffset=0)
{
	Node *n;
	{
		std::lock_guard<std::mutex>lock(engine_mutex);

		n = nodeByPath(remotename.c_str());
		if(!n)
			return -ENOENT;


		{
			auto it = file_cache.find(remotename);
			if(it != file_cache.end() && it->second.td >= 0) {

				if(it->second.status == file_cache_row::DOWNLOADING)
				{
					client->tclose(it->second.td);
					it->second.td = -1;
				}
				else
				{
					//the local copy is better and the file is being uploaded
					return 0;
				}
			}
		}



		opend_ret=0;
		int td = client->topen(n->nodehandle, NULL, startOffset,-1, 1);

		if(td < 0)
			return -EIO;
		file_cache[remotename].td = td;
		if(file_cache[remotename].status != file_cache_row::DOWNLOADING)
			file_cache[remotename].status = file_cache_row::INVALID;
	}
	file_cache[remotename].last_modified = n->mtime;
	file_cache[remotename].size = n->size;
	file_cache[remotename].startOffset = startOffset;
	file_cache[remotename].available_bytes = 0;

	printf("opend: aspetto il risultato per il file %s\n",remotename.c_str());
	{
		std::unique_lock<std::mutex> lk(cvm);
		cv.wait(lk, [this] {return opend_ret;});
	}
	return 0;

}


int MegaFuseModel::open(const char *p, struct fuse_file_info *fi)
{
	auto sPath = splitPath(p);
	//if(sPath.second[0] == '.')
		//return -EINVAL;
	if((fi->flags & O_WRONLY) || ((fi->flags & (O_RDWR | O_TRUNC)) == (O_RDWR | O_TRUNC)))
		{
			return create(p,755,fi);
			

		}

	
	printf("flags:%X\n",fi->flags);
	std::lock_guard<std::mutex>lock(api_mutex);
	std::string path(p);


	//auto path = splitPath(std::string(p));
	Node *n;
	{
		std::lock_guard<std::mutex>lock(engine_mutex);

		

		n = nodeByPath(p);

		if(file_cache.find(path) != file_cache.end()) {
			auto it = file_cache.find(std::string(p));
			if((!n || it->second.last_modified >= n->mtime) && it->second.status != file_cache_row::INVALID) {
				it->second.n_clients++;
				return 0;
			} else if(!it->second.n_clients) {
				eraseCacheRow(it);
			} else
				{printf("\n\n\nstatus %d\n\n",it->second.status);return -EAGAIN;}
		}

		if(!n)
			return -ENOENT;
	}

	int opend_ret = enqueueDownload(p,0);

	if(opend_ret < 0)
		return -EIO;

	{
		auto it = file_cache.find(std::string(p));
		std::lock_guard<std::mutex>lock(engine_mutex);

		it->second.n_clients++;
		it->second.handle = n->nodehandle;
	}
	


	return 0;
}



int MegaFuseModel::create(const char *path, mode_t mode, struct fuse_file_info * fi)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	std::lock_guard<std::mutex>lock2(engine_mutex);

	printf("-------requested create %s\n",path);
	auto p = path;
	
	if((fi->flags & O_EXCL) && (file_cache.find(p)!= file_cache.end() || nodeByPath(p) != nullptr))
		return -EEXIST;
	
	//workaround for dolphin, disable locks completely
	if((fi->flags & O_EXCL) )
	{
		printf("-----------exclusive file forbidden\n");
		return -EPERM;
		
	}
		
	auto sPath = splitPath(p);
	//if(sPath.second[0] == '.')
		//return -EINVAL;				
			file_cache[p].status = file_cache_row::AVAILABLE;
			file_cache[p].size=0;
			file_cache[p].modified=true;
			file_cache[p].n_clients++;
			if(file_cache[p].status == file_cache_row::DOWNLOADING)
				client->tclose(file_cache[p].td);
			truncate(file_cache[p].localname.c_str(), 0);
			return 0;

	
}


int MegaFuseModel::write(const char * path, const char *buf, size_t size, off_t offset, struct fuse_file_info * fi)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	std::lock_guard<std::mutex>lock2(engine_mutex);
	auto it = file_cache.find(path);

	chmod(it->second.localname.c_str(),S_IWUSR|S_IRUSR);
	printf("write, file %s, apro cache: %s\n",it->first.c_str(),it->second.localname.c_str());
	int fd = ::open(it->second.localname.c_str(),O_WRONLY);
	if (fd < 0 )
		return fd;
	int s = pwrite(fd,buf,size,offset);
	close(fd);
	it->second.modified=true;
	int newsize = offset + size;
	if(it->second.size < newsize)
	{
		it->second.size = newsize;
		int numChunks = ceil(float(it->second.size) / CHUNKSIZE );
		printf("file resized to %d\n",newsize);
				it->second.availableChunks.resize(numChunks,false);
		
	}
	return s;
}


int MegaFuseModel::read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("read requested, offset %ld, size %ld\n",offset,size);
	std::lock_guard<std::mutex>lock(api_mutex);

	engine_mutex.lock();

	printf("apro cache: %s\n",file_cache[path].localname.c_str());
	auto it = file_cache.find(path);
    bool dataReady = it->second.status != file_cache_row::DOWNLOADING || chunksAvailable(path,offset,size);
    bool canWait = ((offset+size) - (it->second.available_bytes)) < 1024*1024 && it->second.startOffset <= offset;
    engine_mutex.unlock();

    if(!dataReady)
	{
        if(!canWait)
        {
            printf("--------------read too slow, downloading the requested chunk\n");
			int startOffset = (CHUNKSIZE)* (offset / (CHUNKSIZE));
			int opend_ret = enqueueDownload(path,startOffset);
			if(opend_ret < 0)
				return opend_ret;
        }

        printf("mi metto in attesa di ricevere i dati necessari\n");
		if(!chunksAvailable(path,offset,size)) {

			{
				std::unique_lock<std::mutex> lk(cvm);

				printf("lock acquisito, \n");
				cv.wait(lk, [this,it,path,offset,size] {return !(it->second.status == file_cache_row::DOWNLOADING && !chunksAvailable(path,offset,size));});
				printf("wait conclusa\n");
			}

		}

	}


	int fd = ::open(file_cache[path].localname.c_str(),O_RDONLY);
	if (fd < 0 ) {
		printf("open fallita in read per il file %s\n",file_cache[path].localname.c_str());
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


int MegaFuseModel::mkdir(const char * p, mode_t mode)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	
}
int MegaFuseModel::unlink(std::string filename)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	printf("-------------unlinking %s\n",filename.c_str());
	if(engine_mutex.try_lock()) {
		printf("mutex error in model::unlink\n");
		abort();
	}
		auto it = file_cache.find(filename);
		
		if(it == file_cache.end())
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
	auto it = file_cache.begin();
	while(it != file_cache.end())
		it = eraseCacheRow(it);

}
