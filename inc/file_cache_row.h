#include <string>
#include <vector>
#include <unordered_map>
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

class CacheManager
{
	public: //only for now
	typedef std::unordered_map <std::string,file_cache_row> mapType;
	mapType file_cache;
	public:
	CacheManager();
	file_cache_row& operator[] (std::string);
	
	mapType::iterator find(std::string);
	mapType::iterator end();
	mapType::iterator begin();
	mapType::const_iterator cend();
	mapType::const_iterator cbegin();
	mapType::iterator erase(mapType::const_iterator it);
	size_t size();
};