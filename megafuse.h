#include "megacli.h"
#include <thread>

struct file_stat
{
    int mode;
    int size;
    int nlink;
    int error;
    bool dir;
    time_t mtime;
    file_stat() : error(0){};
};

class MegaFuseCallback : public DemoApp
{
    public:
    void login_result(error e);
    error last_error;
    int result;
    std::mutex login_lock;
    std::mutex download_lock;
    std::mutex open_lock;
    std::mutex unlink_lock;
    std::mutex upload_lock;
    std::mutex putnodes_lock;
    void transfer_failed(int, string&, error);
    void transfer_complete(int, chunkmac_map*, const char*);
    void transfer_failed(int,  error);
    void transfer_complete(int td, handle uploadhandle, const byte* uploadtoken, const byte* filekey, SymmCipher* key);
    void topen_result(int td, error e);
    void topen_result(int td, string* filename, const char* fa, int pfa);
    void unlink_result(handle, error);
    void putnodes_result(error, targettype, NewNode*);

};

class MegaFuse
{
    public:
    MegaFuseCallback megaFuseCallback;
    private:
    static void event_loop(MegaFuse* megaFuse);
    std::thread event_loop_thread;
    std::mutex engine_mutex;
    std::mutex api_mutex;
    byte pwkey[SymmCipher::KEYLENGTH];
    std::pair<std::string,std::string> splitPath(std::string);
    Node* nodeByPath(std::string path);
    Node* childNodeByName(Node *p,std::string name);
    int open(std::string);
    public:
    bool start();
    bool stop();
    bool login(std::string username, std::string password);
    bool download(std::string,std::string dst);
    bool upload(std::string,std::string dst);
    int unlink(std::string);
    int mkdir(std::string);
    std::vector<std::string> ls(std::string path);
    file_stat getAttr(std::string path);
};
