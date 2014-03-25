/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>



int hello_rename(const char * src, const char *dst);

int hello_getattr(const char *path, struct stat *stbuf);
int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int hello_open(const char *path, struct fuse_file_info *fi);
int hello_release(const char *path, struct fuse_file_info *fi);

int hello_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi);
int hello_create(const char *path, mode_t mode, struct fuse_file_info * fi);

int hello_unlink(const char *path);

//STUB
int hello_utimens(const char *a, const struct timespec tv[2]){return 0;};
int hello_chmod(const char *a, mode_t m){return 0;}
int hello_chown(const char *a, uid_t u, gid_t g){return 0;}
int hello_truncate(const char *a, off_t o){return 0;}
int hello_write(const char *, const char *, size_t, off_t, struct fuse_file_info *);
int hello_mkdir(const char *, mode_t);
static struct fuse_operations hello_oper = {
	.getattr	= hello_getattr,
	.readdir	= hello_readdir,
	.open		= hello_open,
	.read		= hello_read,
	.release    = hello_release,
	.unlink     = hello_unlink,
	.rmdir      = hello_unlink,
	.create     =hello_create,
	.utimens    = hello_utimens,
	.chmod      = hello_chmod,
	.chown      = hello_chown,
	.truncate   = hello_truncate,
	.write      = hello_write,
	.mkdir      = hello_mkdir,
	.rename     =hello_rename,
};


int megafuse_main(int argc,char**argv)
{
    return fuse_main(argc, argv, &hello_oper, NULL);

}

