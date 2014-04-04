#include "megacli.h"
#include <thread>
#include <mutex>
#include <fuse.h>
#include <condition_variable>




struct file_cache_row
{
    enum CacheStatus{INVALID,DOWNLOADING,UPLOADING,AVAILABLE,DOWNLOAD_PAUSED};
    std::string localname;
	std::string tmpname;
    int td;
    CacheStatus status;
    int size;
    int available_bytes;
    int n_clients;
    time_t last_modified;
    int startOffset;
    bool modified;
	std::vector<bool> availableChunks;
	uint64_t handle;
    file_cache_row();
    ~file_cache_row();
};

class MegaFuseCallback : public DemoApp
{
    public:


};
#include "EventsHandler.h"
class MegaFuseModel : public DemoApp
{
	static const int CHUNKSIZE = 128*1024;
    typedef std::map <std::string,file_cache_row> cacheMap;
    std::map <std::string,file_cache_row> file_cache;
public:
	MegaFuseModel(EventsHandler &,std::mutex &engine_mutex);
	std::mutex &engine_mutex;
	Node* nodeByPath(std::string path);
	    std::pair<std::string,std::string> splitPath(std::string);

private:
	EventsHandler &eh;
    //static void event_loop(MegaFuseModel* megaFuse);
    std::thread event_loop_thread;
    
    std::mutex api_mutex;
    byte pwkey[SymmCipher::KEYLENGTH];
    
    Node* childNodeByName(Node *p,std::string name);
	bool chunksAvailable(std::string filename,int startOffset,int size);
    public:
	int enqueueDownload(std::string filename,int startOffset);
    int unlink(std::string);
    std::map <std::string,file_cache_row>::iterator findCacheByTransfer(int, file_cache_row::CacheStatus);
    ~MegaFuseModel();
    void check_cache();
    std::map <std::string,file_cache_row>::iterator eraseCacheRow(std::map <std::string,file_cache_row>::iterator it);
    std::map <std::string,file_cache_row>::iterator findCacheByHandle(uint64_t);
	int numChunks(m_off_t p);
	int blockOffset(int pos);
	/*callbacks*/

    error last_error;
    int result;
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
	//void rename_result(handle, error); NOT WORKING
    void putfa_result(handle, fatype, error);
	void nodes_updated(Node**, int);
    /*inter thread*/
    std::mutex cvm;
    std::condition_variable cv;
    int opend_ret;
    int putnodes_ret;
    int unlink_ret;
	void users_updated(User** u, int count);

    /*fuse*/
    int open(const char *path, struct fuse_file_info *fi);
    std::set<std::string> readdir(const char *path);
    int getAttr(const char *path, struct stat *stbuf);
    int release(const char *path, struct fuse_file_info *fi);
    int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
    int write(const char * path, const char *buf, size_t size, off_t offset, struct fuse_file_info * fi);
    int rename(const char * src, const char *dst);



};
