#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "megaclient.h"
#include "megafuse.h"


static MegaFuse* megaFuse;




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

int hello_utimens(const char *a, const struct timespec tv[2]){return 0;};
int hello_chmod(const char *a, mode_t m){return 0;}
int hello_chown(const char *a, uid_t u, gid_t g){return 0;}
int hello_truncate(const char *a, off_t o){return 0;}

extern "C" int megafuse_main(int argc,char**argv);




int megafuse_main(int argc,char**argv)
{
    return 0;

}
static struct fuse_operations hello_oper;

int megafuse_mainpp(int argc,char**argv,MegaFuse* mf)
{
    megaFuse = mf;

	hello_oper.getattr	= hello_getattr;
	hello_oper.readdir	= hello_readdir;
	hello_oper.open		= hello_open;
	hello_oper.read		= hello_read;
	hello_oper.release    = hello_release;
	hello_oper.unlink     = hello_unlink;
	hello_oper.rmdir      = hello_unlink;
	hello_oper.create     =hello_create;
	hello_oper.utimens    = hello_utimens;
	hello_oper.chmod      = hello_chmod;
	hello_oper.chown      = hello_chown;
	hello_oper.truncate   = hello_truncate;
	hello_oper.write      = hello_write;
	hello_oper.mkdir      = hello_mkdir;
	hello_oper.rename     =hello_rename;
    return fuse_main(argc, argv, &hello_oper, NULL);

}
