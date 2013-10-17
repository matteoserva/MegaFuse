#include <string>
#include <map>
#include <time.h>


struct file_cache_data
{

    int fd;
    time_t last_modified;
};


class FileCache
{

    std::map<std::string,file_cache_data> file_cache;
    void safe_erase(std::string);
    void cache_clean();
    public:
    int try_open(std::string, time_t);
    void save_cache(std::string path, int fd, time_t last_modified);
    void invalidate_cache(std::string path);
};
