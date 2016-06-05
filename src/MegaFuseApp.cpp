/* This class will handle the messages coming from the mega client and update the model.
 * 
 **/

#include "mega.h"

#include "megacrypto.h"
#include "megaclient.h"
#include "megafusemodel.h"

#include <sys/types.h>
#include <fcntl.h>

MegaFuseApp::MegaFuseApp(MegaFuseModel* m):model(m)
{

}

MegaFuseApp::~MegaFuseApp()
{

}

void MegaFuseApp::login_result(error e)
{
	int login_ret = (e)? -1:1;
	if(!e)
		client->fetchnodes();
	model->eh.notifyEvent(EventsHandler::LOGIN_RESULT,login_ret);
}

void MegaFuseApp::nodes_updated(Node** n, int c)
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
		auto it = model->cacheManager.find(fullpath);
		auto currentCache = model->cacheManager.findByHandle(n[i]->nodehandle);
		if( !removed && currentCache != model->cacheManager.end() && fullpath != currentCache->first) {
			//the handle is in cache



			printf("rename detected from %s to %s and source is in cache\n",currentCache->first.c_str(),fullpath.c_str());
			model->rename(currentCache->first.c_str(),fullpath.c_str());
		} else if(!removed && it!= model->cacheManager.end() && it->second.status == file_cache_row::UPLOADING) {
			printf("file uploaded nodehandle %lx\n",n[i]->nodehandle);
			it->second.handle = n[i]->nodehandle;
			it->second.status = file_cache_row::AVAILABLE;
			it->second.last_modified = n[i]->mtime;
			model->eh.notifyEvent(EventsHandler::NODE_UPDATED,0,fullpath);

		}

		else if(!removed && it!= model->cacheManager.end()) {
			printf("file overwritten. nodehandle %lx\n",n[i]->nodehandle);
			it->second.handle = n[i]->nodehandle;
			it->second.status = file_cache_row::INVALID;
			model->eh.notifyEvent(EventsHandler::NODE_UPDATED,0,fullpath);

		} else if(removed && currentCache != model->cacheManager.end()) {
			printf("unlink detected, %s\n",currentCache->first.c_str());
			model->unlink(currentCache->first);

		}



	}
}



void MegaFuseApp::putnodes_result(error e , targettype , NewNode* nn)
{
	delete[] nn;
	model->eh.notifyEvent(EventsHandler::PUTNODES_RESULT,(e)?-1:1);
}

void MegaFuseApp::topen_result(int td, error e)
{
	printf("topen fallito!\n");
	client->tclose(td);
	for(auto it = model->cacheManager.begin(); it!=model->cacheManager.end(); ++it)
		if(it->second.td == td)
			it->second.td = -1;
	model->eh.notifyEvent(EventsHandler::TOPEN_RESULT,e);
}


//transfer opened for download
void MegaFuseApp::topen_result(int td, string* filename, const char* fa, int pfa)
{
		printf("topen riuscito\n");
	std::string remotename = "";
	std::string tmp;
	for(auto it = model->cacheManager.begin(); it!=model->cacheManager.end(); ++it)
		if(it->second.td == td) {
			remotename = it->first;
			tmp = it->second.localname;

			if(it->second.status == file_cache_row::INVALID) {
				it->second.availableChunks.clear();
				it->second.availableChunks.resize(CacheManager::numChunks(it->second.size),false);
			}
		}
	//
	client->dlopen(td,tmp.c_str());
	printf("file: %s ora in stato DOWNLOADING\n",remotename.c_str());
	model->cacheManager[remotename].status = file_cache_row::DOWNLOADING;
	model->cacheManager[remotename].available_bytes = 0;
	model->cacheManager[remotename].td = td;
	model->eh.notifyEvent(EventsHandler::TOPEN_RESULT,+1);
}

//download completed
void MegaFuseApp::transfer_complete(int td, chunkmac_map* macs, const char* fn)
{
	printf("\ndownload complete\n\n");
	auto it = model->cacheManager.findByTransfer(td,file_cache_row::DOWNLOADING );
	if(it == model->cacheManager.end())
		return;
	
	client->tclose(it->second.td);
	it->second.td = -1;

	int missingOffset = it->second.firstUnavailableOffset();
	
	if(missingOffset < 0) {
		std::string remotename = it->first;

			it->second.status = file_cache_row::AVAILABLE;
		model->eh.notifyEvent(EventsHandler::TRANSFER_COMPLETE,+1);

		printf("download complete %s, transfer %d\n",remotename.c_str(),td);
	} else {

		int startOffset = missingOffset;
		int startBlock = CacheManager::numChunks(startOffset);
		int neededBytes = -1;
		for(unsigned int i = startBlock; i < it->second.availableChunks.size(); i++) {
			if(it->second.availableChunks[i]) {
				//workaround 2, download a bit more because the client is not always updated at the block boundary
				neededBytes = ChunkedHash::SEGSIZE + CacheManager::blockOffset(i) - startOffset;
				break;
			}
		}
		
		if(startOffset +neededBytes > it->second.size)
			neededBytes = -1;
		printf("------download reissued, missing %d bytes starting from block %d\n",neededBytes,startBlock);
		Node*n = model->nodeByPath(it->first);
		int td = client->topen(n->nodehandle, NULL, startOffset,neededBytes, 1);
		if(td < 0)
			return;
		it->second.td = td;
		it->second.startOffset = startOffset;
		//it->second.status = file_cache_row::INVALID;
		it->second.available_bytes=0;

	}
}

void MegaFuseApp::transfer_complete(int td, handle ulhandle, const byte* ultoken, const byte* filekey, SymmCipher* key)
{
	auto it = model->cacheManager.findByTransfer(td,file_cache_row::UPLOADING );
	if(it == model->cacheManager.end()) {
		client->tclose(td);
		return;
	}

	printf("upload riuscito\n");

	auto sPath = model->splitPath(it->first);

	Node *target = model->nodeByPath(sPath.first);
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
	model->eh.notifyEvent(EventsHandler::UPLOAD_COMPLETE,1);

}

void MegaFuseApp::transfer_failed(int td, error e)
{
	printf("upload fallito\n");
	client->tclose(td);
	auto it = model->cacheManager.findByTransfer(td, file_cache_row::UPLOADING);
	if(it == model->cacheManager.end()) {
		it->second.status = file_cache_row::AVAILABLE;
		it->second.td = -1;
	}
	model->eh.notifyEvent(EventsHandler::UPLOAD_COMPLETE,e);
}

void MegaFuseApp::transfer_failed(int td, string& filename, error e)
{
	printf("download fallito: %d\n",e);
	auto it = model->cacheManager.findByTransfer(td,file_cache_row::DOWNLOADING );
	client->tclose(td);

	it->second.status = file_cache_row::INVALID;
	model->eh.notifyEvent(EventsHandler::TRANSFER_COMPLETE,-1);
}

void MegaFuseApp::transfer_update(int td, m_off_t bytes, m_off_t size, dstime starttime)
{
	std::string remotename = "";
	if(model->cacheManager.findByTransfer(td,file_cache_row::UPLOADING ) != model->cacheManager.end()) {
		cout << '\r'<<"UPLOAD TD " << td << ": Update: " << bytes/1024 << " KB of " << size/1024 << " KB, " << bytes*10/(1024*(client->httpio->ds-starttime)+1) << " KB/s" ;
		fflush(stdout);
		return;
	}
	
	auto it = model->cacheManager.findByTransfer(td,file_cache_row::DOWNLOADING );
	if(it == model->cacheManager.end())
	{
		//no file found
		client->tclose(td);
		return;
	}
	
	

	int startChunk = CacheManager::numChunks(it->second.startOffset);

	it->second.available_bytes = ChunkedHash::chunkfloor(ChunkedHash::chunkfloor(it->second.startOffset) + bytes);

	int endChunk = CacheManager::numChunks(ChunkedHash::chunkfloor(it->second.available_bytes));
	if(it->second.startOffset + bytes >= size)
	{
		it->second.available_bytes = size;
		endChunk = it->second.availableChunks.size();
	}

	for(int i = startChunk; i < endChunk; i++) {
		try {
			if(!it->second.availableChunks[i]) {
				it->second.availableChunks[i] = true;
				
				printf("block %d/%d available\n",i,(int)it->second.availableChunks.size());

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
	
	

	model->eh.notifyEvent(EventsHandler::TRANSFER_UPDATE,0);

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

void MegaFuseApp::unlink_result(handle h, error e)
{
	printf("unlink eseguito\n");
	int unlink_ret  = (e)?-1:1;
	model->eh.notifyEvent(EventsHandler::UNLINK_RESULT,unlink_ret);
}

void MegaFuseApp::users_updated(User** u, int count)
{
	DemoApp::users_updated(u,count);
	model->eh.notifyEvent(EventsHandler::USERS_UPDATED);
}
