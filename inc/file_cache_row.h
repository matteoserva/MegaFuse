#include <string>
#include <vector>

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
	
	bool canRead(size_t offset,size_t size);
	bool chunksAvailable(int startOffset,int size);
	static int numChunks(size_t pos);
};
