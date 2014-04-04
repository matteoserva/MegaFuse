#include "file_cache_row.h"
#include <fcntl.h>
#include <unistd.h>
file_cache_row::file_cache_row(): td(-1),status(INVALID),size(0),available_bytes(0),n_clients(0),startOffset(0),modified(false),handle(0)
{
	localname = tmpnam(NULL);
	tmpname = tmpnam(NULL);
	int fd = open(localname.c_str(),O_CREAT|O_WRONLY,S_IWUSR|S_IRUSR);
	close(fd);
	fd = open(tmpname.c_str(),O_CREAT|O_WRONLY,S_IWUSR|S_IRUSR);
	close(fd);
	printf("creato il file %s\n",localname.c_str());


}
file_cache_row::~file_cache_row()
{

}