#ifndef _megafusemodel_h_
#define _megafusemodel_h_

#include "megacli.h"
#include <thread>
#include <mutex>
#include <fuse.h>
#include <condition_variable>

#include "file_cache_row.h"
#include <unordered_map>

#include "EventsHandler.h"
const char* errorstring(error e);
class MegaFuseModel;

class MegaFuseApp : public DemoApp
{
private:
	MegaFuseModel* model;
public:

	MegaFuseApp(MegaFuseModel* m);
	~MegaFuseApp();

	void login_result(error e);
    void transfer_failed(int, string&, error);
    void transfer_complete(int, chunkmac_map*, const char*);
    void transfer_failed(int,  error);
    void transfer_complete(int td, handle uploadhandle, const byte* uploadtoken, const byte* filekey, SymmCipher* key);
    void topen_result(int td, error e);
    void topen_result(int td, string* filename, const char* fa, int pfa);
    void unlink_result(handle, error);
    void putnodes_result(error, targettype, NewNode*);
    void transfer_update(int, m_off_t, m_off_t, dstime);
	void nodes_updated(Node**, int);
	void users_updated(User** u, int count);
};

class MegaFuseModel
{
	friend class MegaFuseApp;
	CacheManager cacheManager;
public:
	MegaFuseModel(EventsHandler &,std::mutex &engine_mutex);
	std::mutex &engine_mutex;
	Node* nodeByPath(std::string path);
	std::pair<std::string,std::string> splitPath(std::string);
	int makeAvailableForRead(const char *path, off_t offset,size_t size);



private:
	EventsHandler &eh;

    Node* childNodeByName(Node *p,std::string name);
    public:
    int unlink(std::string);
    std::unordered_map <std::string,file_cache_row>::iterator findCacheByTransfer(int, file_cache_row::CacheStatus);
    ~MegaFuseModel();
    void check_cache();
    std::unordered_map <std::string,file_cache_row>::iterator findCacheByHandle(uint64_t);
	MegaApp* getCallbacksHandler();
	MegaFuseApp callbacksHandler;

    /*fuse*/
    int open(const char *path, struct fuse_file_info *fi);
    std::set<std::string> readdir(const char *path);
    int getAttr(const char *path, struct stat *stbuf);
    int release(const char *path, struct fuse_file_info *fi, bool makethumb = true);
    int releaseNoThumb(const char *path, struct fuse_file_info *fi);
    int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
    int write(const char * path, const char *buf, size_t size, off_t offset, struct fuse_file_info * fi);
    int rename(const char * src, const char *dst);
};



#endif
