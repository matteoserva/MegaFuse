#include "file_cache_row.h"
#include <fcntl.h>
#include <unistd.h>

#include <megaclient.h>


file_cache_row::file_cache_row(): td(-1),status(INVALID),size(0),available_bytes(0),n_clients(0),startOffset(0),modified(false),handle(0)
{
	localname = tmpnam(NULL);
	int fd = open(localname.c_str(),O_CREAT|O_WRONLY,S_IWUSR|S_IRUSR);
	close(fd);

	printf("creato il file %s\n",localname.c_str());


}
file_cache_row::~file_cache_row()
{
	::unlink(localname.c_str());
}


/* Returns true if a read should not block, 
 * it doesn't matter if a read will result in an error or in a successful read
 */
bool file_cache_row::canRead(size_t offset,size_t size)
{
	return status != file_cache_row::DOWNLOADING || chunksAvailable(offset,size);
	
	
}



bool file_cache_row::chunksAvailable(int startOffset,int size)
{
	int startChunk = numChunks(ChunkedHash::chunkfloor(startOffset));//startOffset/CHUNKSIZE;
	int endChunk = numChunks(startOffset+size);
	bool available = true;
	for(int i = startChunk; i < endChunk && i < (int) availableChunks.size(); i++) {
		available = available && availableChunks[i];
	}
	//printf("chunksavailable da %d a %d, blocchi da %d a %d escluso. riferimento: %d\n",startOffset,startOffset+size,startChunk,endChunk,ChunkedHash::chunkfloor(startOffset));
	return available;
}


int file_cache_row::numChunks(size_t pos)
{
	size_t end = 0;
	if(pos == 0)
		return 0;
	for(int i=1;i<=8;i++)
        {
                end +=i*ChunkedHash::SEGSIZE;
                if(end >= pos) return i;
        }
        return 8 + ceil(float(pos-end)/(8.0*ChunkedHash::SEGSIZE));
	
	
}

CacheManager::CacheManager()
{
	
}

file_cache_row& CacheManager::operator[] (std::string s)
{
	return file_cache[s];
	
}

CacheManager::mapType::iterator CacheManager::find(std::string s)
{
	return file_cache.find(s);
}
	CacheManager::mapType::iterator CacheManager::end()
	{
		return file_cache.end();
	}
	CacheManager::mapType::iterator CacheManager::begin()
	{
		return file_cache.begin();
	}
		CacheManager::mapType::const_iterator CacheManager::cend()
	{
		return file_cache.cend();
	}
	CacheManager::mapType::const_iterator CacheManager::cbegin()
	{
		return file_cache.cbegin();
	}
	
	CacheManager::mapType::iterator CacheManager::erase(CacheManager::mapType::const_iterator it)
	{
		return file_cache.erase(it);
	}

	size_t CacheManager::size()
	{
		return file_cache.size();
	}