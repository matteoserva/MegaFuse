#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "megaclient.h"
#include "MegaFuse.h"

static MegaFuse* megaFuse;
#include <mutex>

static std::mutex api_mutex;

int hello_write(const char * path, const char *buf, size_t size, off_t offset, struct fuse_file_info * fi)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	return megaFuse->write(path,buf,size,offset,fi);
}
int hello_getattr(const char *path, struct stat *stbuf)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	return megaFuse->getAttr(path,stbuf);
}
int hello_open(const char *path, struct fuse_file_info *fi)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	return megaFuse->open(path,fi);
}
int hello_read(const char *path, char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	//int s = pread(fi->fh,buf,size,offset);

	return megaFuse->read(path,buf,size,offset,fi);
}
int hello_rename(const char * src, const char *dst)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	return megaFuse->rename(src,dst);
}
int hello_unlink(const char *path)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	return megaFuse->unlink(path);
}
int hello_release(const char *path, struct fuse_file_info *fi)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	return megaFuse->release(path,fi);
}
int hello_mkdir(const char * path, mode_t mode)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	return megaFuse->mkdir(path,mode);

}
int hello_create(const char *path, mode_t mode, struct fuse_file_info * fi)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	return megaFuse->create(path,mode,fi);
}
int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                  off_t offset, struct fuse_file_info *fi)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	return megaFuse->readdir(path,buf,filler,offset,fi);
}

int hello_utimens(const char *a, const struct timespec tv[2])
{
	return 0;
};
int hello_chmod(const char *a, mode_t m)
{
	return 0;
}
int hello_chown(const char *a, uid_t u, gid_t g)
{
	return 0;
}
int hello_truncate(const char *a, off_t o)
{
	return megaFuse->truncate(a,o);
}
int hello_link(const char *a, const char *b)
{
	//return megaFuse->rename(b,a);
	//printf("link %s %s\n",a,b);
	return -EMLINK;
}
int hello_statvfs(const char * a, struct statvfs * stat)
{
	stat->f_blocks  = 50*1024*1024;
	stat->f_bavail  = 49*1024*1024;
	stat->f_bsize   = 1024;
	stat->f_namemax = 256;

	return 0;
} 
int hello_setxattr(const char *path, const char *name,
			const char *value, size_t size, int flags)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	return megaFuse->setxattr(path, name, value, size, flags);
}
int hello_getxattr(const char *path, const char *name,
                       char *value, size_t size)
{
	return megaFuse->getxattr(path, name, value, size);
}
int hello_symlink(const char *path1, const char *path2)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	return megaFuse->symlink(path1, path2);
}
int hello_readlink(const char *path, char *buf, size_t bufsiz)
{
	std::lock_guard<std::mutex>lock(api_mutex);
	return megaFuse->readlink(path,buf,bufsiz);
}
int megafuse_mainpp(int argc,char**argv,MegaFuse* mf)
{
	megaFuse = mf;
	struct fuse_operations hello_oper = {0};
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
	hello_oper.symlink    = hello_symlink;
	hello_oper.write      = hello_write;
	hello_oper.mkdir      = hello_mkdir;
	hello_oper.rename     =hello_rename;
	hello_oper.link	      = hello_link;
	hello_oper.statfs    = hello_statvfs;
	hello_oper.readlink	= hello_readlink;
//#ifdef HAVE_SETXATTR
	hello_oper.setxattr	= hello_setxattr;
	hello_oper.getxattr	= hello_getxattr;
//	hello_oper.listxattr	= hello_listxattr;
//	hello_operremovexattr	= hello_removexattr;
//#endif

	return fuse_main(argc, argv, &hello_oper, NULL);
}
