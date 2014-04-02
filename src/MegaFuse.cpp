


#include "megaclient.h"
#include "megafusemodel.h"
#include "Config.h"
#include <curl/curl.h>
#include "megacrypto.h"
#include <termios.h>
#include "megaposix.h"

#include <db_cxx.h>
#include "megabdb.h"
#include "MegaFuse.h"
MegaFuse::MegaFuse()
{

	model = new MegaFuseModel(eh,engine_mutex);
	client = new MegaClient(model,new CurlHttpIO,new BdbAccess,Config::getInstance()->APPKEY.c_str());
	event_loop_thread = std::thread(event_loop,this);
}
void MegaFuse::event_loop(MegaFuse* that)
{


	for (;;) {
		that->engine_mutex.lock();
		client->exec();
		that->engine_mutex.unlock();
		client->wait();
	}
}
bool MegaFuse::login()
{
	std::string username = Config::getInstance()->USERNAME.c_str();
	std::string password = Config::getInstance()->PASSWORD.c_str();
	model->engine_mutex.lock();
	client->pw_key(password.c_str(),pwkey);
	client->login(username.c_str(),pwkey,1);
	model->engine_mutex.unlock();
	{
		EventsListener el(eh,EventsHandler::LOGIN_RESULT);
		EventsListener eu(eh,EventsHandler::USERS_UPDATED);
		auto l_res = el.waitEvent();
		if (l_res.result <0)
			return false;
		eu.waitEvent();
	}
	return true;
}

int MegaFuse::create(const char* path, mode_t mode, fuse_file_info* fi)
{
	std::lock_guard<std::mutex>lock2(engine_mutex);
	return model->create(path,mode,fi);
}

int MegaFuse::getAttr(const char* path,struct stat* stbuf)
{
	std::lock_guard<std::mutex>lock2(engine_mutex);
	if(model->getAttr(path,stbuf) == 0) //file locally cached
		return 0;


	Node *n = model->nodeByPath(path);
	if(!n)
		return -ENOENT;
	switch (n->type) {
	case FILENODE:
		printf("filenode richiesto\n");
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		stbuf->st_size = n->size;
		stbuf->st_mtime = n->ctime;
		break;

	case FOLDERNODE:
	case ROOTNODE:
		printf("rootnode richiesto\n");
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = 4096;
		stbuf->st_mtime = n->ctime;
		break;
	default:
		printf("einval nodo\n");
		return -EINVAL;
	}


	return 0;
}

int MegaFuse::mkdir(const char* p, mode_t mode)
{
	std::unique_lock<std::mutex> l(engine_mutex);


	auto path = model->splitPath(p);
	std::string base = path.first;
	std::string newname = path.second;

	Node* n = model->nodeByPath(base.c_str());
	SymmCipher key;
	string attrstring;
	byte buf[Node::FOLDERNODEKEYLENGTH];
	NewNode* newnode = new NewNode[1];

	// set up new node as folder node
	newnode->source = NEW_NODE;
	newnode->type = FOLDERNODE;
	newnode->nodehandle = 0;
	newnode->mtime = newnode->ctime = time(NULL);
	newnode->parenthandle = UNDEF;

	// generate fresh random key for this folder node
	PrnGen::genblock(buf,Node::FOLDERNODEKEYLENGTH);
	newnode->nodekey.assign((char*)buf,Node::FOLDERNODEKEYLENGTH);
	key.setkey(buf);

	// generate fresh attribute object with the folder name
	AttrMap attrs;
	attrs.map['n'] = newname;

	// JSON-encode object and encrypt attribute string
	attrs.getjson(&attrstring);
	client->makeattr(&key,&newnode->attrstring,attrstring.c_str());

	// add the newly generated folder node

	EventsListener el(eh,EventsHandler::PUTNODES_RESULT);

	client->putnodes(n->nodehandle,newnode,1);
	l.unlock();
	auto l_res = el.waitEvent();
	if(l_res.result < 0)
		return -EIO;
	return 0;
}

int MegaFuse::open(const char* path, fuse_file_info* fi)
{
	return model->open(path,fi);
}

int MegaFuse::read(const char* path, char* buf, size_t size, off_t offset, fuse_file_info* fi)
{
	return model->read(path,buf,size,offset,fi);
}

int MegaFuse::readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, fuse_file_info* fi)
{
	std::lock_guard<std::mutex>lockE(engine_mutex);

	Node* n = model->nodeByPath(path);
	if(!n) {
		return -EIO;
	}
	if(!(n->type == FOLDERNODE || n->type == ROOTNODE))
		return -EIO;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	std::set<std::string> names = model->readdir(path);

	for (node_list::iterator it = n->children.begin(); it != n->children.end(); it++)
		names.insert((*it)->displayname());
	for(auto it = names.begin(); it != names.end(); ++it)
		filler(buf,it->c_str(),NULL,0);




	return 0;
}


int MegaFuse::release(const char* path, fuse_file_info* fi)
{
	return model->release(path,fi);
}

int MegaFuse::rename(const char* src, const char* dst)
{
	std::unique_lock<std::mutex>lockE(engine_mutex);
	Node * n_src = model->nodeByPath(src);
	Node * n_dst = model->nodeByPath(dst);

	if(n_src) {
		auto path = model->splitPath(dst);
		Node *dstFolder = model->nodeByPath(path.first);

		if(!dstFolder)
			return -EINVAL;

		if ( client->rename(n_src,dstFolder) != API_OK)
			return -EIO;

		n_src->attrs.map['n'] = path.second.c_str();
		if (client->setattr(n_src))
			return -EIO;
	} else {
		if(model->rename(src,dst) < 0)
			return -ENOENT;

	}
	//delete overwritten file
	if(n_dst && n_dst->type == FILENODE) {
		EventsListener el(eh,EventsHandler::UNLINK_RESULT);
		lockE.unlock();
		client->unlink(n_dst);
		auto l_res = el.waitEvent();
	}


	return 0;


}

int MegaFuse::write(const char* path, const char* buf, size_t size, off_t offset, fuse_file_info* fi)
{
	std::lock_guard<std::mutex>lock2(engine_mutex);
	return model->write(path,buf,size,offset,fi);
}
int MegaFuse::unlink(const char* s)
{
	printf("-----------unlink %s\n",s);
	{
		std::lock_guard<std::mutex>lock(engine_mutex);
		Node *n = model->nodeByPath(s);
		if(!n)
			return model->unlink(s);

		error e = client->unlink(n);
		if(e)
			return -EINVAL;
	}
	{
		EventsListener el(eh,EventsHandler::UNLINK_RESULT);
		auto l_res = el.waitEvent();
		if(l_res.result < 0)
			return -EIO;
		return 0;
	}



	return 0;

}
