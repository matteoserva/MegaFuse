#include "megaclient.h"
#include "megafusemodel.h"

void MegaFuseModel::users_updated(User** u, int count)
{
	DemoApp::users_updated(u,count);
	eh.notifyEvent(EventsHandler::USERS_UPDATED);
}

void MegaFuseModel::putnodes_result(error e, targettype, NewNode* nn)
{
	delete[] nn;
	eh.notifyEvent(EventsHandler::PUTNODES_RESULT,(e)?-1:1);

}

void MegaFuseModel::transfer_failed(int td,  error e)
{
	printf("upload fallito\n");
	last_error = e;
	client->tclose(td);
	auto it = findCacheByTransfer(td, file_cache_row::UPLOADING);
	if(it == cacheManager.end()) {
		it->second.status = file_cache_row::AVAILABLE;
		it->second.td = -1;
	}
	eh.notifyEvent(EventsHandler::UPLOAD_COMPLETE,-1);


	//eh.notifyEvent(EventsHandler::NODE_UPDATED,-1);
	//  upload_lock.unlock();
}



std::string last_thumbnail;
SymmCipher last_key;



void MegaFuseModel::transfer_complete(int td, handle ulhandle, const byte* ultoken, const byte* filekey, SymmCipher* key)
{

	//DemoApp::transfer_complete(td,uploadhandle,uploadtoken,filekey,key);
	auto it = findCacheByTransfer(td,file_cache_row::UPLOADING );
	if(it == cacheManager.end()) {
		client->tclose(td);
		return;
	}

	printf("upload riuscito\n");

	auto sPath = splitPath(it->first);

	Node *target = nodeByPath(sPath.first);
	if(!target) {
		cout << "Upload target folder inaccessible, using /" << endl;
		target = client->nodebyhandle(client->rootnodes[0]);
	}
	/*if (!putf->targetuser.size() && !client->nodebyhandle(putf->target)) {
		cout << "Upload target folder inaccessible, using /" << endl;
		putf->target = client->rootnodes[0];
	}*/

	NewNode* newnode = new NewNode[1];

	// build new node
	newnode->source = NEW_UPLOAD;

	// upload handle required to retrieve/include pending file attributes
	newnode->uploadhandle = ulhandle;

	// reference to uploaded file
	memcpy(newnode->uploadtoken,ultoken,sizeof newnode->uploadtoken);

	// file's crypto key
	newnode->nodekey.assign((char*)filekey,Node::FILENODEKEYLENGTH);
	newnode->mtime = newnode->ctime = time(NULL);
	newnode->type = FILENODE;
	newnode->parenthandle = UNDEF;

	AttrMap attrs;

	MegaClient::unescapefilename(&sPath.second);

	attrs.map['n'] = sPath.second;
	std::string localname = it->second.localname;
	attrs.getjson(&localname);

	client->makeattr(key,&newnode->attrstring,localname.c_str());





	/*if (putf->targetuser.size()) {
		cout << "Attempting to drop file into user " << putf->targetuser << "'s inbox..." << endl;
		client->putnodes(putf->targetuser.c_str(),newnode,1);
	} else*/ client->putnodes(target->nodehandle,newnode,1);

	printf("ulhandle %lx, nodehandle %lx\n",ulhandle,newnode->nodehandle);

	it->second.td = -1;
	it->second.modified = false;

	client->tclose(td);
	eh.notifyEvent(EventsHandler::UPLOAD_COMPLETE,1);


}

void MegaFuseModel::putfa_result(handle, fatype, error e)
{
	printf("putfa ricevuto\n\n");
	if(e)
		printf("errore putfa\n");



}

std::unordered_map <std::string,file_cache_row>::iterator MegaFuseModel::findCacheByHandle(uint64_t h)
{
	for(auto it = cacheManager.begin(); it != cacheManager.end(); ++it) {
		if(it->second.handle == h)
			return it;

	}
	return cacheManager.end();


}

void MegaFuseModel::nodes_updated(Node** n, int c)
{
	DemoApp::nodes_updated(n,c);

	if(!n)
		return;

	for(int i = 0; i<c; i++) {
		bool removed = false;;
		Node * nd = n[i];
		std::string fullpath = std::string(nd->displayname());
		while(nd->parent && nd->parent->type == FOLDERNODE) {
			fullpath = std::string(nd->parent->displayname()) + "/" + fullpath;
			nd = nd->parent;
		}
		if(nd->parent->type == ROOTNODE) {
			fullpath = "/" + fullpath;
		} else {
			fullpath = "//" + fullpath;
			removed = true;
		}
		removed = removed || n[i]->removed;

		printf("fullpath %s\n",fullpath.c_str());
		auto it = cacheManager.find(fullpath);
		auto currentCache = findCacheByHandle(n[i]->nodehandle);
		if( !removed && currentCache != cacheManager.end() && fullpath != currentCache->first) {
			//the handle is in cache



			printf("rename detected from %s to %s and source is in cache\n",currentCache->first.c_str(),fullpath.c_str());
			rename(currentCache->first.c_str(),fullpath.c_str());
		} else if(!removed && it!= cacheManager.end() && it->second.status == file_cache_row::UPLOADING) {
			printf("file uploaded nodehandle %lx\n",n[i]->nodehandle);
			it->second.handle = n[i]->nodehandle;
			it->second.status = file_cache_row::AVAILABLE;
			it->second.last_modified = n[i]->mtime;
			eh.notifyEvent(EventsHandler::NODE_UPDATED,0,fullpath);

		}

		else if(!removed && it!= cacheManager.end()) {
			printf("file overwritten. nodehandle %lx\n",n[i]->nodehandle);
			it->second.handle = n[i]->nodehandle;
			it->second.status = file_cache_row::INVALID;
			eh.notifyEvent(EventsHandler::NODE_UPDATED,0,fullpath);

		} else if(removed && currentCache != cacheManager.end()) {
			printf("unlink detected, %s\n",currentCache->first.c_str());
			unlink(currentCache->first);

		}



	}
}

void MegaFuseModel::topen_result(int td, error e)
{

	printf("topen fallito!\n");

	eh.notifyEvent(EventsHandler::TOPEN_RESULT,-1);
	

}

void MegaFuseModel::unlink_result(handle h, error e)
{

	printf("unlink eseguito\n");
	int unlink_ret  = (e)?-1:1;
	eh.notifyEvent(EventsHandler::UNLINK_RESULT,unlink_ret);

	//unlink_lock.unlock();
}
// topen() succeeded (download only)
void MegaFuseModel::topen_result(int td, string* filename, const char* fa, int pfa)
{
	last_error = API_OK;
	result = td;
	printf("topen riuscito\n");



	std::string remotename = "";
	std::string tmp;
	for(auto it = cacheManager.begin(); it!=cacheManager.end(); ++it)
		if(it->second.td == td) {
			remotename = it->first;
			tmp = it->second.localname;
			/*int fdt = ::open(tmp.c_str(), O_TRUNC, O_WRONLY);
			if(fdt >= 0)
				close(fdt);*/
			if(it->second.status == file_cache_row::INVALID) {

				it->second.availableChunks.clear();
				
				it->second.availableChunks.resize(numChunks(it->second.size),false);


			}
		}
	chmod(tmp.c_str(),S_IWUSR|S_IRUSR);
	client->dlopen(td,tmp.c_str());
	printf("file: %s ora in stato DOWNLOADING\n",remotename.c_str());
	cacheManager[remotename].status = file_cache_row::DOWNLOADING;
	cacheManager[remotename].available_bytes = 0;
	eh.notifyEvent(EventsHandler::TOPEN_RESULT,+1);
}

void MegaFuseModel::transfer_failed(int td, string& filename, error e)
{

	printf("download fallito: %d\n",e);
	auto it = findCacheByTransfer(td,file_cache_row::DOWNLOADING );
	client->tclose(td);

		it->second.status = file_cache_row::INVALID;
	eh.notifyEvent(EventsHandler::TRANSFER_COMPLETE,-1);

	//DemoApp::transfer_failed(td,filename,e);
	last_error=e;
	//download_lock.unlock();
}

std::unordered_map <std::string,file_cache_row>::iterator MegaFuseModel::findCacheByTransfer(int td, file_cache_row::CacheStatus status)
{
	for(auto it = cacheManager.begin(); it!=cacheManager.end(); ++it)
		if(it->second.status == status && it->second.td == td)
			return it;
	return cacheManager.end();
}




/*
  macs and fn must be ignored.
*/
void MegaFuseModel::transfer_complete(int td, chunkmac_map* macs, const char* fn)
{
	printf("\ndownload complete\n\n");
	auto it = findCacheByTransfer(td,file_cache_row::DOWNLOADING );
	if(it == cacheManager.end())
		return;
	
	client->tclose(it->second.td);
	it->second.td = -1;

	bool complete = true;
	for (bool i : it->second.availableChunks ) {
		complete = complete && i;
	}
	if(complete) {
		std::string remotename = it->first;

			it->second.status = file_cache_row::AVAILABLE;
		eh.notifyEvent(EventsHandler::TRANSFER_COMPLETE,+1);

		printf("download riuscito per %s,%d\n",remotename.c_str(),td);
		last_error=API_OK;
	} else {

		int startBlock = 0;
		for(unsigned int i = 0; i < it->second.availableChunks.size(); i++) {
			if(!it->second.availableChunks[i]) {
				startBlock = i;
				break;
			}
		}
		int startOffset = blockOffset(startBlock);
		int neededBytes = -1;
		for(unsigned int i = startBlock; i < it->second.availableChunks.size(); i++) {
			if(it->second.availableChunks[i]) {
				//workaround 2, download a bit more because the client is not always updated at the block boundary
				neededBytes = ChunkedHash::SEGSIZE + blockOffset(i) - startOffset;
				break;
			}
		}
		
		if(startOffset +neededBytes > it->second.size)
			neededBytes = -1;
		printf("------download reissued, missing %d bytes starting from block %d\n",neededBytes,startBlock);
		Node*n = nodeByPath(it->first);
		int td = client->topen(n->nodehandle, NULL, startOffset,neededBytes, 1);
		if(td < 0)
			return;
		it->second.td = td;
		it->second.startOffset = startOffset;
		//it->second.status = file_cache_row::INVALID;
		it->second.available_bytes=0;

	}


	//download_lock.unlock();
}

void MegaFuseModel::transfer_update(int td, m_off_t bytes, m_off_t size, dstime starttime)
{


	std::string remotename = "";

	
	if(findCacheByTransfer(td,file_cache_row::UPLOADING ) != cacheManager.end()) {
		cout << '\r'<<"UPLOAD TD " << td << ": Update: " << bytes/1024 << " KB of " << size/1024 << " KB, " << bytes*10/(1024*(client->httpio->ds-starttime)+1) << " KB/s" ;

		fflush(stdout);
		return;
	}
	auto it = findCacheByTransfer(td,file_cache_row::DOWNLOADING );
	if(it == cacheManager.end())
	{
		client->tclose(td);
		return;
	}
	
	

	int startChunk = numChunks(it->second.startOffset);

	it->second.available_bytes = ChunkedHash::chunkfloor(ChunkedHash::chunkfloor(it->second.startOffset) + bytes);

	int endChunk = numChunks(ChunkedHash::chunkfloor(it->second.available_bytes));
	if(it->second.startOffset + bytes >= size)
	{
		it->second.available_bytes = size;
		endChunk = it->second.availableChunks.size();
	}

	bool shouldStop = false;
	for(int i = startChunk; i < endChunk; i++) {
		try {
			if(!it->second.availableChunks[i]) {
				it->second.availableChunks[i] = true;
				void *buf = malloc(1024*1024);
				int fd;
				fd = ::open(it->second.tmpname.c_str(),O_RDONLY);
				int daLeggere = blockOffset(i+1)-blockOffset(i);
				int s = pread(fd,buf,daLeggere,blockOffset(i));
				close(fd);
				fd = ::open(it->second.localname.c_str(),O_WRONLY);
				pwrite(fd,buf,s,blockOffset(i));
				close(fd);
				free(buf);
				printf("blocco %d/%d disponibile, copiati %d/%d byte a partire da %d\n",i,it->second.availableChunks.size(),s,daLeggere,blockOffset(i));

			}

		} catch(...) {
			printf("errore, ho provato a leggere il blocco %d\n",i);
			fflush(stdout);
			abort();
		}
	}


	/*	cout << remotename << td << ": Update: " << bytes/1024 << " KB of " << size/1024 << " KB, " << bytes*10/(1024*(client->httpio->ds-starttime)+1) << " KB/s" << endl;
		cout << "scaricato fino al byte " <<(it->second.startOffset+bytes) << " di: "<<size<<endl;
    */
	{
		std::string rigaDownload = "\r[";
		
		int numero = it->second.availableChunks.size();
		for(int i = 0;i<50;i++)
		{
			if(it->second.availableChunks[i*numero/50])
				rigaDownload.append("#");
			else
				rigaDownload.append("-");
		}
		rigaDownload.append("] ");
		
		
		
		//std::cout << "readable bytes: " <<  ChunkedHash::chunkfloor(ChunkedHash::chunkfloor(it->second.startOffset) + bytes) <<std::endl;
		static time_t last_update= time(NULL);
		if(last_update < time(NULL))
		{
		std::cout << rigaDownload << size/1024/1024 << " MB, " << bytes*10/(1024*(client->httpio->ds-starttime)+1) << " KB/s, available_b/size " <<it->second.available_bytes<<"/"<<it->second.size<< " "<<(it->second.startOffset+bytes)<< "/" <<size<<"            ";
		last_update = time(NULL);
		}
	}
	
	

	eh.notifyEvent(EventsHandler::TRANSFER_UPDATE,0);

	if( it->second.n_clients<=0) {
		client->tclose(it->second.td);
		it->second.status =file_cache_row::DOWNLOAD_PAUSED;
		it->second.td = -1;
		printf("pausing download\n");
	}

	//WORKAROUNDS

	if(it->second.startOffset && it->second.available_bytes>= it->second.size) {
		printf("workaround 1\n"); //have to call manually if the download didn't start at 0
		transfer_complete(td,NULL,NULL);
	} else if(endChunk > startChunk && endChunk < it->second.availableChunks.size() && it->second.availableChunks[endChunk]) {
		printf("----Downloading already available data at block %d. stopping...\n",endChunk);
		transfer_complete(td,NULL,NULL);
	}



}

void MegaFuseModel::login_result(error e)
{
	printf("risultato arrivato\n");
	int login_ret = (e)? -1:1;

	if(!e)
		client->fetchnodes();
	eh.notifyEvent(EventsHandler::LOGIN_RESULT,login_ret);
	printf("fine login_result\n");
}
