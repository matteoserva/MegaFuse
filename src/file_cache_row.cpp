/*  These classes will handle the cached file info.
 *
 */

#include "file_cache_row.h"
#include <fcntl.h>
#include <unistd.h>

#include <megaclient.h>
#include <sys/stat.h>

file_cache_row::file_cache_row(): td(-1),status(INVALID),size(0),available_bytes(0),n_clients(0),startOffset(0),modified(false),handle(0)
{
	localname = tmpnam(NULL);
	int fd = open(localname.c_str(),O_CREAT|O_WRONLY,S_IWUSR|S_IRUSR);
	close(fd);
	chmod(localname.c_str(),S_IWUSR|S_IRUSR);
	printf("creato il file %s\n",localname.c_str());
	
	
}
file_cache_row::~file_cache_row()
{

}


/* Returns true if a read should not block,
 * it doesn't matter if a read will result in an error or in a successful read
 */
bool file_cache_row::canRead(size_t offset,size_t size)
{
	return status != file_cache_row::DOWNLOADING || chunksAvailable(offset,size);
	
	
}

int file_cache_row::firstUnavailableOffset()
{
	for(unsigned int i = 0; i < availableChunks.size(); i++)
		if(!availableChunks[i])
			return CacheManager::blockOffset(i);
	return -1;
	
}


/*tells if the chunks required to perform a read are available*/
bool file_cache_row::chunksAvailable(int startOffset,int size)
{
	int startChunk = CacheManager::numChunks(ChunkedHash::chunkfloor(startOffset));//startOffset/CHUNKSIZE;
	int endChunk = CacheManager::numChunks(startOffset+size);
	bool available = true;
	for(int i = startChunk; i < endChunk && i < (int) availableChunks.size(); i++) {
		available = available && availableChunks[i];
	}
	//printf("chunksavailable da %d a %d, blocchi da %d a %d escluso. riferimento: %d\n",startOffset,startOffset+size,startChunk,endChunk,ChunkedHash::chunkfloor(startOffset));
	return available;
}

/*return the num of chunks required to store a file of this size*/
int CacheManager::numChunks(size_t pos)
{
	size_t end = 0;
	if(pos == 0)
		return 0;
	for(int i=1; i<=8; i++) {
		end +=i*ChunkedHash::SEGSIZE;
		if(end >= pos) return i;
	}
	return 8 + ceil(float(pos-end)/(8.0*ChunkedHash::SEGSIZE));
	
	
}

/*returns the starting offset of a specified block
 */

int CacheManager::blockOffset(int pos)
{
	m_off_t end = 0;
	
	for(int i=1; i<=8; i++) {
		if(i> pos) return end;
		end +=i*ChunkedHash::SEGSIZE;
	}
	return (pos-8)*8*ChunkedHash::SEGSIZE + end;
}

CacheManager::mapType::iterator CacheManager::findByHandle(uint64_t h)
{
	for(auto it = begin(); it != end(); ++it) {
		if(it->second.handle == h)
			return it;
			
	}
	return end();
	
	
}
CacheManager::mapType::iterator CacheManager::findByTransfer(int td, file_cache_row::CacheStatus status)
{
	for(auto it = begin(); it!=end(); ++it)
		if(it->second.status == status && it->second.td == td)
			return it;
	return end();
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

CacheManager::mapType::iterator CacheManager::tryErase(CacheManager::mapType::iterator it)
{
	if(it->second.n_clients != 0)
		return it;
		
	::unlink(it->second.localname.c_str());
	return file_cache.erase(it);
}

size_t CacheManager::size()
{
	return file_cache.size();
}
