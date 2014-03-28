#include "megacli.h"
#include <thread>
#include <mutex>
#include <fuse.h>
#include <condition_variable>

struct MegaFuseFilePut : public AppFilePut
{
	void start();
    std::string remotename;
	MegaFuseFilePut(const char* fn, handle tn, const char* tu, const char* nn,std::string rn) : AppFilePut(fn,tn,tu,nn) { remotename = rn;}
};

struct file_cache_row
{
    enum CacheStatus{INVALID,DOWNLOADING,UPLOADING,AVAILABLE};
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

    file_cache_row();
    ~file_cache_row();
};

class MegaFuseCallback : public DemoApp
{
    public:


};

class MegaFuse : public DemoApp
{
	static const int CHUNKSIZE = 128*1024;
    typedef std::map <std::string,file_cache_row> cacheMap;
    std::map <std::string,file_cache_row> file_cache;

    public:
    private:
    static void event_loop(MegaFuse* megaFuse);
    std::thread event_loop_thread;
    std::mutex engine_mutex;
    std::mutex api_mutex;
    byte pwkey[SymmCipher::KEYLENGTH];
    std::pair<std::string,std::string> splitPath(std::string);
    Node* nodeByPath(std::string path);
    Node* childNodeByName(Node *p,std::string name);
	bool chunksAvailable(std::string filename,int startOffset,int size);
    public:
    bool start();
    bool stop();
	int enqueueDownload(std::string filename,int startOffset);
    bool login(std::string username, std::string password);
    int unlink(std::string);
    std::map <std::string,file_cache_row>::iterator findCacheByTransfer(int, file_cache_row::CacheStatus);
    ~MegaFuse();
    void check_cache();
    std::map <std::string,file_cache_row>::iterator eraseCacheRow(std::map <std::string,file_cache_row>::iterator it);
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
    void putfa_result(handle, fatype, error);

    /*inter thread*/
    std::mutex cvm;
    std::condition_variable cv;
    int login_ret;
    int opend_ret;
    int putnodes_ret;
    int unlink_ret;

    /*fuse*/
    int open(const char *path, struct fuse_file_info *fi);
    int readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi);
    int getAttr(const char *path, struct stat *stbuf);
    int release(const char *path, struct fuse_file_info *fi);
    int mkdir(const char * path, mode_t mode);
    int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
    int create(const char *path, mode_t mode, struct fuse_file_info * fi);
    int write(const char * path, const char *buf, size_t size, off_t offset, struct fuse_file_info * fi);
    int rename(const char * src, const char *dst);



};
