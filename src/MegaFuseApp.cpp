#include "mega.h"

#include "megacrypto.h"
#include "megaclient.h"
#include "megafusemodel.h"

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

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
			rename(currentCache->first.c_str(),fullpath.c_str());
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

void MegaFuseApp::putfa_result(handle h, fatype f , error e)
{
	return model->putfa_result (h,f,e);
}

void MegaFuseApp::putnodes_result(error a , targettype b , NewNode* c)
{
	return model->putnodes_result(a,b,c);
}

void MegaFuseApp::topen_result(int td, error e)
{
	return model->topen_result(td,e);
}

void MegaFuseApp::topen_result(int td, string* filename, const char* fa, int pfa)
{
	return model->topen_result(td,filename,fa,pfa);
}

void MegaFuseApp::transfer_complete(int a, chunkmac_map* b, const char* c)
{
	return model->transfer_complete(a,b,c);
}

void MegaFuseApp::transfer_complete(int td, handle uploadhandle, const byte* uploadtoken, const byte* filekey, SymmCipher* key)
{
	return model->transfer_complete(td,uploadhandle,uploadtoken,filekey,key);
}

void MegaFuseApp::transfer_failed(int a, error e)
{
	return model->transfer_failed(a,e);
}

void MegaFuseApp::transfer_failed(int a, string& b , error c)
{
	return model->transfer_failed(a,b,c);
}

void MegaFuseApp::transfer_update(int a, m_off_t b , m_off_t c , dstime d)
{
	return model->transfer_update(a,b,c,d);
}

void MegaFuseApp::unlink_result(handle a, error b)
{
	return model->unlink_result(a,b);
}

void MegaFuseApp::users_updated(User** u, int count)
{
	DemoApp::users_updated(u,count);
	model->eh.notifyEvent(EventsHandler::USERS_UPDATED);
}
