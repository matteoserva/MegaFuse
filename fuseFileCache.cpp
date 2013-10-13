#include "fuseFileCache.h"

#define MAX_CACHE_ENTRIES 50
#define MIN_CACHE_ENTRIES 20

int FileCache::try_open(std::string path, time_t if_not_modified_since)
{
    auto it = file_cache.find(path);
    printf("filecache: tempo richiesto: %d\n\n\n",if_not_modified_since);
    if(it != file_cache.end())
    {
        if(it->second.last_modified == if_not_modified_since)
        {
                printf("filecache: cache hit\n");
            return dup(it->second.fd);
        }
    }

    return -1;
}
void FileCache::cache_clean()
{
    if(file_cache.size() < MAX_CACHE_ENTRIES)
        return;
    auto it = file_cache.begin();
    for (int i = 0; i < MIN_CACHE_ENTRIES;++i)
        it = file_cache.erase(it);


}
void FileCache::safe_erase(std::string path)
{
    auto it = file_cache.find(path);
    if(it != file_cache.end())
    {
        close(it->second.fd);
        file_cache.erase(it);
    }
}
void FileCache::save_cache(std::string path, int fd, time_t last_modified)
{
    safe_erase(path);
    cache_clean();
    file_cache[path].last_modified = last_modified;
    file_cache[path].fd = dup(fd);
    return ;
}

void FileCache::invalidate_cache(std::string path)
{
    safe_erase(path);

}
