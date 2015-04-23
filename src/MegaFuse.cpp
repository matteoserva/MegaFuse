/* This class is responsible for querying both the mega remote fs and the local model,
 * then it returns the results.
 * 
 */


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
MegaFuse::MegaFuse():running(true)
{

	model = new MegaFuseModel(eh,engine_mutex);
	MegaApp* handler = model->getCallbacksHandler();
	
	client = new MegaClient(handler,new CurlHttpIO,new BdbAccess,Config::getInstance()->APPKEY.c_str());
	event_loop_thread = std::thread(event_loop,this);

	std::thread maintenance_loop(maintenance,this);
	maintenance_loop.detach();
	printf("MegaFushe::MegaFuse. Constructor finished.\n");
}
void MegaFuse::maintenance(MegaFuse* that)
{
	while(that->running)
	{
		if(that->needSerializeSymlink)
			that->serializeSymlink();

	        if(that->needSerializeXattr)
			that->serializeXattr();

		std::this_thread::sleep_for(std::chrono::milliseconds(60000));
	}
}
void MegaFuse::event_loop(MegaFuse* that)
{
	for (;that->running;) {
		that->engine_mutex.lock();
		client->exec();
		that->engine_mutex.unlock();
		client->wait();
	}
	printf("End of event_loop\n");

	if(that->needSerializeSymlink)
		that->serializeSymlink();
	if(that->needSerializeXattr)
		that->serializeXattr();

	that->running=false;
	that->event_loop_thread.join();
}

MegaFuse::~MegaFuse()
{
	running=false;
	event_loop_thread.join();
	delete model;
}

bool MegaFuse::login()
{
	xattrLoaded = false;
	symlinkLoaded = false;
	needSerializeSymlink = false;
	needSerializeXattr = false;
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
	//std::lock_guard<std::mutex>lock2(engine_mutex);
	printf("----------------CREATE flags:%X\n",fi->flags);
	fi->flags = fi->flags |O_CREAT | O_TRUNC;// | O_WRONLY;
	
	return model->open(path,fi);
}

int MegaFuse::getAttr(const char* path,struct stat* stbuf)
{

	if (!symlinkLoaded)
		unserializeSymlink();

	if (!symlinkLoaded)
		return -ENOSPC;

        std::string str_path(path);

	std::lock_guard<std::mutex>lock2(engine_mutex);

	printf("MegaFuse::getAttr Looking for %s\n",str_path.c_str());
	if(model->getAttr(path,stbuf) == 0) //file locally cached
		//just if not a symlink
		if (modelist.find(str_path) ==  modelist.end())
			return 0;


	Node *n = model->nodeByPath(path);
	if(!n)
		return -ENOENT;



	switch (n->type) {
	case FILENODE:
		if ((modelist.find(str_path) !=  modelist.end())) { //symlink
			stbuf->st_mode = modelist[str_path]; 
		} else { //regular file
			stbuf->st_mode = S_IFREG | 0666;
		}
		stbuf->st_nlink = 1;
		stbuf->st_size = n->size;
		stbuf->st_mtime = n->ctime;
		break;

	case FOLDERNODE:
	case ROOTNODE:
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = 4096;
		stbuf->st_mtime = n->ctime;
		break;
	default:
		printf("invalid node\n");
		return -EINVAL;
	}


	return 0;
}
int MegaFuse::truncate(const char *a,off_t o)
{
	fuse_file_info fi;
	fi.flags = O_TRUNC;
	if(!open(a,&fi))
		release(a,&fi);
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
	//FIXME: the engine is running between the two calls
	int res = model->makeAvailableForRead(path,offset,size);
	if(res < 0){
		printf("MegaFuse::read. Not available for read\n");
		return res;
	}
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
int MegaFuse::releaseNoThumb(const char* path, fuse_file_info* fi)
{
	return model->release(path,fi, false);
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
int MegaFuse::write(const char *path, const void *buf, size_t size, 
		off_t offset, struct fuse_file_info * fi)
{
	return write(path, (const char *)buf, size, offset, fi);
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
//implementation of extended attributes
int MegaFuse::setxattr(const char *path, const char *name,
			const char *value, size_t size, int flags)
{
	needSerializeXattr = false;

	if (!xattrLoaded)       // we need to prevent this happend
		return -ENOSPC; // it could happend while file still
                                // downloading and we cannot acces file yet

	int ret = raw_setxattr(path, name, value, size, flags);

	if (xattrLoaded)//just if we have been unserialized values
		needSerializeXattr = true;

	return ret;
}
int MegaFuse::raw_setxattr(const char *path, const char *name,
			const char *value, size_t size, int flags)
{
	std::string str_path(path);
	std::string str_name(name);

	printf("MegaFuse::raw_setxattr. setting path '%s', "
		"name '%s', value '%s' with size '%d'\n", 
		path, name, value, size);
	printf("MegaFuse::raw_setxattr. setting path '%x', "
		"name '%x', value '%x' with size '%d'\n", 
		path, name, value, size);

	if (xattr_list.find(str_path) != xattr_list.end()) {
		printf ("MegaFuse::raw_setxattr. Path exist.\n");
		if (xattr_list[str_path].find(name) ==
		    xattr_list[str_path].end()) {
			printf ("MegaFuse::raw_setxattr. "
				"name don't exist, creating.\n");
			xattr_list[str_path][str_name] = new Xattr_value;
		}
	} else {
		printf ("MegaFuse::raw_setxattr. "
			"Path NO exist. Setting new value.\n");
		xattr_list[str_path][str_name] = new Xattr_value;
	}

	size_t path_t = strlen(path)+1; //strlen gives chars count without \0
	xattr_list[str_path][str_name]->path = new char[path_t];
	memcpy (xattr_list[str_path][str_name]->path, 
		path, 
		path_t); //the null terminator should be copied

	size_t name_t = strlen(name)+1;
	xattr_list[str_path][str_name]->name = new char[name_t];
	memcpy (xattr_list[str_path][str_name]->name,
		name,
		name_t);  

	xattr_list[str_path][str_name]->value = new char[size];
	memcpy(xattr_list[str_path][str_name]->value, value, size);

	xattr_list[str_path][str_name]->size = size;	

	return 0;
}
int MegaFuse::getxattr(const char *path, const char *name,
			char *value, size_t size)
{
	if (!xattrLoaded)
		unserializeXattr();

	if (!xattrLoaded)
		return -ENODATA;

	std::string str_path(path);
	std::string str_name(name);

	printf("MegaFuse::getxattr. looking for path '%s', "
		"name '%s'\n", path, name);
	printf("MegaFuse::getxattr. looking for path '%x', "
		"name '%x'\n", path, name);

	if (xattr_list.find(str_path) !=  xattr_list.end()) {
		printf("MegaFuse::getxattr path found, looking for name\n");
		if (xattr_list[str_path].find(str_name) !=
		    xattr_list[str_path].end()) {
			//according to doc, if size is zero, we return size of value
			//without set value
			if (size == 0) {
				return (int)xattr_list[str_path][str_name]->size;
			}

			memcpy (value,	xattr_list[str_path][str_name]->value, 
				(int)xattr_list[str_path][str_name]->size);

			return (int)xattr_list[str_path][str_name]->size;
                
		} else { 
			printf("getxattr. No name '%s' "
				"found on getxattr\n", name); 
		}
	} else { 
		printf("getxattr. No path '%s'(%x) "
			"found on getxattr\n", path, path);
	}

	printf("getxattr. Warning. attribute "
		"not found: %s on path %s\n", name, path);
	return -ENODATA;
}
// implementation of symlinks 
// (getAttr should return S_IFLNK byte when symlink are found)
int MegaFuse::symlink(const char *path1, const char *path2)
{
	if (!symlinkLoaded)
		unserializeSymlink();

	if (!symlinkLoaded)
		return -ENOSPC;

        std::string str_path2(path2);
	std::string str_path1(path1);
	unsigned int size = str_path1.size();

	mode_t mode; //not used by create func
	fuse_file_info *fi = new fuse_file_info;

	std::unique_lock<std::mutex>lockE(engine_mutex);
	Node *n = model->nodeByPath(std::string(path2));
	lockE.unlock();

	if (!(!n)) {
		//here, file exist on MEGA
		printf("MegaFuse::symlink. Removing existent file link\n");
		int res = (int)unlink(path2);
	}

	fi->flags = O_WRONLY;
	create(path2, mode, fi);

	fi->flags = O_WRONLY | O_CREAT | O_TRUNC;
	write(path2, path1,size, 0, fi);
	releaseNoThumb(path2,fi);

	printf("MegaFuse::symlink. Save %s on modelist\n", str_path2.c_str());
	modelist[str_path2] = S_IFLNK | 0666;

	needSerializeSymlink = true;

	delete fi;
        return 0;
}
int MegaFuse::readlink(const char *path, char *buf, size_t bufsiz)
{
	int res;
	unsigned int size;

	char *tmpbuf = new char[bufsiz];
	memset(tmpbuf, '\0', bufsiz);

	fuse_file_info *fi = new fuse_file_info;
	fi->flags = O_CLOEXEC;
	res = (int)open(path, fi);
	fi->flags = O_RDONLY;

	res = (int)read(path, tmpbuf, bufsiz, 0, fi);
	printf("MegaFuse::readlink. read res: %d\n");

	memcpy (buf, tmpbuf , (int)bufsiz);

	printf("MegaFuse::readlink. buf size: %d\n", bufsiz);
	printf("MegaFuse::readlink. buf value: %s\n", buf);

	delete[] tmpbuf;
	delete fi;
	if (res > 0) {
		return 0;
	} else {
		return res;
	}
}
//TODO: Make it on binary mode like serializeXattr
int MegaFuse::serializeSymlink()
{
	std::string sep = ";";
	std::string tmpbuf;
	std::stringstream line;

	mode_t mode; //not used by create func
	fuse_file_info *fi = new fuse_file_info;
	const char *path = "/.megafuse_symlink.temp";
	const char *path_final = "/.megafuse_symlink";
	const char *path_old = "/.megafuse_symlink.old";
	fi->flags = O_WRONLY;
	std::unique_lock<std::mutex>lockE(engine_mutex);
	Node *n = model->nodeByPath(std::string(path_final));
	lockE.unlock();

	if (!n)	{
		printf("Nothing yet here\n");
	} else {
		//here, file exist on MEGA
		printf("MegaFuse::serializeSymlink. "
			"Node EXIST on Mega? Unlink file.\n");
		rename(path_final, path_old);
	}

	fi->flags = O_WRONLY;
	create(path, mode, fi);
	fi->flags = O_APPEND;

	for (map<std::string, mode_t>::iterator iter = modelist.begin();
	     iter != modelist.end();
	     ++iter) {	
		//this could be more simple but keep in mind store also mode of file
		line << iter->first << endl;
		printf("MegaFuse::serializeSymlink. "
			"New line to serialize %s.\n", line.str().c_str());

		if ((tmpbuf.size()+line.str().size()) >= MEGAFUSE_BUFF_SIZE) {
			stringToFile(path, tmpbuf, tmpbuf.size(), fi);
			tmpbuf.clear(); // clear our temporal buffer
		}

		tmpbuf += line.str();
		line.clear(); // clear lines to be filled again
		line.str(std::string());
	}

	printf("MegaFuse::serializeSymlink. Calling last stringToFile\n");
	stringToFile(path, tmpbuf, tmpbuf.size(), fi);
	tmpbuf.clear();

	releaseNoThumb(path,fi);
	rename(path, path_final);

	needSerializeSymlink = false;

	delete fi;
	return 0;
}
//serialization of xattr and symlinks
int MegaFuse::serializeXattr()
{
	mode_t mode; //not used by create func
	fuse_file_info *fi = new fuse_file_info;
	const char *path = "/.megafuse_xattr.temp";
	const char *path_final = "/.megafuse_xattr";
	const char *path_old = "/.megafuse_xattr.old";
	void *binbuff = malloc(MEGAFUSE_BUFF_SIZE);
	uint32_t buffsize = 0;
	uint32_t data_size;
	fi->flags = O_WRONLY;

	std::unique_lock<std::mutex>lockE(engine_mutex);
	Node *n = model->nodeByPath(std::string(path_final));
	lockE.unlock();

	if (!n)	{
		//here file could not exist on MEGA but exist on cache (/tmp/mega.xxx)
		printf("MegaFuse::serializeXattr."
			"Node no exist on MEGA (could exist on cache)\n");
	} else {
		//here, file exist on MEGA. We should unserialize this?
		printf("MegaFuse::serializeXattr. "
			"Node EXIST? (on MEGA), unlink.\n");
		rename(path_final, path_old);
	}

	fi->flags = O_WRONLY; //just to prevent error on printf on create func
	create(path, mode, fi); //this will trunk file
	fi->flags = O_APPEND; //open file in append mode, no overwrite

	for (Map_outside::iterator iter = xattr_list.begin(); 
	     iter != xattr_list.end(); 
	     ++iter) {
		for (Map_inside::iterator iter2 = iter->second.begin(); 
		     iter2 != iter->second.end(); 
		     ++iter2) {
			printf("MegaFuse::serializeXatr New xattr %s "
				"on path %s "
				"with value %.*s\n", 
				iter2->second->name,
				iter2->second->path,
				iter2->second->size,
				iter2->second->value);

			data_size = (uint32_t)((strlen(iter2->second->path)+1));//+1 copy also \0
			if ((buffsize + sizeof(uint32_t) + data_size) >=
			    MEGAFUSE_BUFF_SIZE) {
				write(path, binbuff, buffsize, 0, fi);
				buffsize = 0;
			}
			addChunk(binbuff, &buffsize, iter2->second->path, data_size);

			data_size = (uint32_t)((strlen(iter2->second->name)+1));
			if ((buffsize + sizeof(uint32_t) + data_size) >=
			    MEGAFUSE_BUFF_SIZE) {
				write(path, binbuff, buffsize, 0, fi);
				buffsize = 0;
			}
			addChunk(binbuff, &buffsize, iter2->second->name, data_size);

			if ((buffsize + sizeof(uint32_t) + iter2->second->size) >=
			    MEGAFUSE_BUFF_SIZE) {
				write(path, binbuff, buffsize, 0, fi);
				buffsize = 0;
			}
			addChunk(binbuff,
				&buffsize, 
				iter2->second->value, 
				(uint32_t)iter2->second->size);
		}
	}

	printf("MegaFuse::serializeXattr: Calling stringToFile.\n");
	write(path, binbuff, buffsize, 0, fi);

	releaseNoThumb(path,fi);
	rename(path, path_final);
	releaseNoThumb(path_final,fi); //this will upload the file

	needSerializeXattr = false;

	delete fi;
	free(binbuff);
	return 0;
}
uint32_t MegaFuse::addChunk(void *binbuff, uint32_t *buffsize, 
			char *data, uint32_t data_size)
{
	printf("MegaFuse::addChunk  data_size:%d "
		"buffsize:%d "
		"allsize:%d\n", 
		data_size,
		*buffsize,
		(*buffsize + sizeof(uint32_t) + data_size));

	printf("MegaFuse::addChunk memcpy1\n");
	memcpy(binbuff + *buffsize, &data_size, sizeof(uint32_t));

	printf("MegaFuse::addChunk memcpy2\n");
	memcpy(binbuff + *buffsize + sizeof(uint32_t), data, data_size);
	
	*buffsize = *buffsize + sizeof(uint32_t) + data_size;
	return *buffsize;
}
int MegaFuse::unserializeSymlink()
{
	char *buffer = new char[MEGAFUSE_BUFF_SIZE];
	size_t size = MEGAFUSE_BUFF_SIZE;
	int num_byt_read = 1;
	bool isdata = false;
	fuse_file_info *fi = new fuse_file_info; //not really used
	const char *path = "/.megafuse_symlink";
	char *it = NULL;
	
	int sizetmp = 0;
	int err;
	off_t offset = 0;
	std::stringstream line;
	std::vector< int > sizes;
	
	//force download file into cache
	fi->flags = O_CLOEXEC;
	int res = (int)open(path, fi);
	fi->flags = O_RDONLY;
	if (res < 0) {
		delete[] buffer;
		delete fi;
		delete it;

		if (res == -2) { //file not found, we dont have xattr yet
			printf("MegaFuse::unserializeSymlink. "
				"no symlink stored. Do nothing.\n");			
			symlinkLoaded = !symlinkLoaded;
			return res;
		}

		printf("MegaFuse::unserializeSymlink. "
			"ERROR We cannot unserialize symlinks! (%d):(\n", res);
		return res;
	}
	
	memset(buffer, '\0', MEGAFUSE_BUFF_SIZE);
	while (num_byt_read > 0) {
		memset(buffer, '\0', MEGAFUSE_BUFF_SIZE);
		num_byt_read = (int)read(path, buffer, size, offset, fi);
		//end if there is nothing to read
		if ((int)num_byt_read <= 0) {
			if ((int)num_byt_read == 0)
				symlinkLoaded = !symlinkLoaded;
			delete[] buffer;
			delete fi;
			return num_byt_read;
			break;
		}
		offset = offset + size;

		it = buffer;
		while (*it) {
			if (*it == '\n') {
				printf("MegaFuse::unserializeSymlink. "
					"Save %s on modelist\n",
					line.str().c_str());

				modelist[line.str()] = S_IFLNK | 0666;

				line.clear();
				line.str(std::string());

			} else {
				line << *it;
			}

			*it++;
		}
	}

	delete[] buffer;
	delete fi;
	return -1;
}
int MegaFuse::unserializeXattr()
{
	if(xattrLoaded)
		return 0;

	char *buffer = new char[MEGAFUSE_BUFF_SIZE];
	size_t size = MEGAFUSE_BUFF_SIZE;
	int readed_b = 1;
	bool isdata = false;
	int i = 0;
	int e = 0;
	fuse_file_info *fi = new fuse_file_info; //not really used
	const char *path = "/.megafuse_xattr";
	uint32_t copied_b = 0;
	uint32_t bytes_to_copy;
	char *data[3] = { NULL }; //0->path 1->name 2->value

	off_t offset = 0;

	//force download file into cache
	fi->flags = O_CLOEXEC;
	int res = (int)open(path, fi);
	fi->flags = O_RDONLY;
	if (res < 0) {
		delete[] buffer;
		delete fi;

		if (res == -2) { //file not found, we dont have xattr yet
			printf("MegaFuse::unserializeXattr. "
				"No xattr stored. Do nothing.\n");
			xattrLoaded = true;
			return res;
		}

		printf("MegaFuse::unserializeXattr. "
			"ERROR We cannot unserialize xattr! (%d):(\n", res);
		return res;
	}
	
	//read file in MEGAFUSE_BUFF_SIZE chunks
	for (readed_b = 1;
	     readed_b > 0;  
	     readed_b = (int)read(path, buffer, size, offset, fi)) {
		//Error on data, prevent infinite loop
		//TODO: An infinite loop may occur after e=0; FIX IT
		if (e > 10) {
			break;
			printf("MegaFuse::unserializeXattr. "
				"ERROR. INCONSISTENT DATA\n");
		}
		copied_b = 0;
		while ((readed_b - copied_b) > 0 && copied_b < readed_b) {
			if (!isdata) {
				if ((readed_b - copied_b) < sizeof(uint32_t)) {//insufficient readed bytes
					e++;
					break;//break forces re-read file starting at offset
				}
				e=0;

				//TODO: be carefully on little-endian vs big-endian
				memcpy(&bytes_to_copy, buffer+copied_b,
				       sizeof(uint32_t));//num of bytes to copy
				copied_b = copied_b+sizeof(uint32_t);
				isdata = !isdata;
			} else {
				if ((readed_b - copied_b) < bytes_to_copy) {//insufficient readed bytes
					e++;
					break;//break forces re-read file starting at offset
				}
				e=0;

				if (data[i] != NULL)
					free(data[i]);

				data[i] = (char *) malloc(bytes_to_copy);
				memcpy(data[i], (char *)buffer+copied_b, 
				       bytes_to_copy);
				copied_b = copied_b+bytes_to_copy;

				//all data recolected 0->path 1->name 2->value
				//the last bytes_to_copy is the size of value
				if (i == 2) {
					raw_setxattr(data[0], data[1], data[2],
						     bytes_to_copy, 0);
					i=-1;
				}
				isdata = !isdata;
				i++;

				if ((readed_b - copied_b) == 0) {
					e++;
					break;
				}
				e=0;
			}//if-else
		}//while
		offset = offset + copied_b;
	}//for

	delete[] buffer;
	delete fi;
	if(data[0] != NULL)
		free(data[0]);
	if(data[1] != NULL)
		free(data[1]);
	if(data[2] != NULL)
		free(data[2]);
	xattrLoaded = true;

	return 0;
}
int MegaFuse::stringToFile(const char *path, std::string tmpbuf, 
			size_t bufsize, fuse_file_info *fi)
{
	if(bufsize > MEGAFUSE_BUFF_SIZE)
		return -1;

	char *buffer = new char[MEGAFUSE_BUFF_SIZE +1];

	printf("stringToFile writting to file: %s\n", tmpbuf.c_str());
	memset(buffer, '\0', bufsize); // reset buffer
	memcpy(buffer, (char *)tmpbuf.c_str(), bufsize); // copy our lines to buffer
	buffer[MEGAFUSE_BUFF_SIZE] = '\0';
	write(path, buffer, tmpbuf.size(), 0, fi); // send the buffer to write function

	delete[] buffer;
	return 0;
}

