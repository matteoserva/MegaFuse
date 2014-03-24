#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "megaclient.h"
#include "megafuse.h"
#include "fuseFileCache.h"
static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";

static MegaFuse* megaFuse;
static FileCache fileCache;



extern "C" int hello_write(const char * path, const char *buf, size_t size, off_t offset, struct fuse_file_info * fi)
{

    return megaFuse->write(path,buf,size,offset,fi);
}

extern "C" int hello_getattr(const char *path, struct stat *stbuf)
{
	return megaFuse->getAttr(path,stbuf);
}

extern "C" int hello_open(const char *path, struct fuse_file_info *fi)
{
    return megaFuse->open(path,fi);
}

extern "C" int hello_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	//int s = pread(fi->fh,buf,size,offset);

	return megaFuse->read(path,buf,size,offset,fi);
}

extern "C" int hello_rename(const char * src, const char *dst)
{
    return megaFuse->rename(src,dst);
}
extern "C" int hello_unlink(const char *path)
{
    return megaFuse->unlink(path);
}
extern "C" int hello_release(const char *path, struct fuse_file_info *fi)
{
    return megaFuse->release(path,fi);
}


extern "C" int hello_mkdir(const char * path, mode_t mode)
{
    return megaFuse->mkdir(path,mode);

}
extern "C" int hello_create(const char *path, mode_t mode, struct fuse_file_info * fi)
{
    return megaFuse->create(path,mode,fi);
}



extern "C" int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	return megaFuse->readdir(path,buf,filler,offset,fi);
}

extern "C" int megafuse_main(int argc,char**argv);
int megafuse_mainpp(int argc,char**argv,MegaFuse* mf)
{
    megaFuse = mf;
    return megafuse_main(argc, argv);

}
