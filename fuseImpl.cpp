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

struct file_info
{
    std::string remotename;
    std::string localname;
    bool modified;
    bool file_exists;
    file_info(): modified(false),file_exists(false){};
};

std::map<uint64_t,file_info> open_files;

extern "C" int hello_write(const char * path, const char *buf, size_t size, off_t offset, struct fuse_file_info * fi)
{
    int s = pwrite(fi->fh,buf,size,offset);
    open_files[fi->fh].modified = true;

    return s;
}

extern "C" int hello_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
    printf("fuse:getattr\n");
	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, hello_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(hello_str);
	} else
		//res = -ENOENT;
    {
        file_stat fs = megaFuse->getAttr(std::string(path));
        if(fs.error)
        {
            for(auto it = open_files.cbegin();it != open_files.cend();++it)
            {
                if(it->second.remotename == std::string(path))
                {
                    stbuf->st_mode = 0444 | S_IFREG;
                    stbuf->st_nlink = 1;
                    stbuf->st_size = 0;
                    return res;
                }
            }

            return fs.error;
        }

        stbuf->st_mode = fs.mode | (fs.dir?S_IFDIR:S_IFREG);
        stbuf->st_nlink = fs.nlink;
        stbuf->st_size = fs.size;
        stbuf->st_mtime = fs.mtime;
    }
	return res;
}

extern "C" int hello_open(const char *path, struct fuse_file_info *fi)
{
    struct stat stbuf;
    int attrRes = hello_getattr(path,&stbuf);
    int fd;


    if(attrRes)
        return -ENOENT;

    char *tmp = tmpnam(NULL);
    if((fd = fileCache.try_open(path,stbuf.st_mtime)) > 0)
    {
        char buffer[256];
        sprintf(buffer,"%s%d%s%d","/proc/",getpid(),"/fd/",fd);
        printf("hack: %s\n",buffer);

        symlink(buffer,tmp);
                    open_files[fd].localname = tmp;
        if(fi->flags & (O_WRONLY |O_RDWR))
            ftruncate(fd, 0);

    }
    else
    {

        auto result = megaFuse->download(std::string(path),tmp);

        printf("fuse: apro %s\n",tmp);
        if (!result )
            return -ENOENT;
        chmod(tmp, S_IRUSR | S_IWUSR);
        fd = open (tmp,O_RDWR);

        if (fd < 0)
            return -EIO;

        fileCache.save_cache(path,fd,stbuf.st_mtime);
            open_files[fd].localname = tmp;

    }
    fi->fh = fd;
    open_files[fd].remotename = path;
    open_files[fd].file_exists = true;
	return 0;
}

extern "C" int hello_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	int s = pread(fi->fh,buf,size,offset);

	return s;
}


extern "C" int hello_unlink(const char *path)
{
    return megaFuse->unlink(path);
}
extern "C" int hello_release(const char *path, struct fuse_file_info *fi)
{
    printf("fuse: release chiamato\n");

    if(open_files[fi->fh].modified)
    {
        if(open_files[fi->fh].file_exists)
            hello_unlink(path);
        fileCache.invalidate_cache(path);
        megaFuse->upload(open_files[fi->fh].localname,open_files[fi->fh].remotename);
    }
    close(fi->fh);
    if(open_files[fi->fh].localname != "" )
        unlink(open_files[fi->fh].localname.c_str());
    open_files.erase(fi->fh);
    return 0;
}


extern "C" int hello_mkdir(const char * path, mode_t)
{
    return megaFuse->mkdir(path);

}
extern "C" int hello_create(const char *path, mode_t mode, struct fuse_file_info * fi)
{
    char *tmp = tmpnam(NULL);

    printf("fuse: create %s\n",tmp);
    int fd = open (tmp,O_RDWR | O_CREAT,S_IRUSR | S_IWUSR);

    if (fd < 0)
        return -EIO;

    fi->fh = fd;
    open_files[fd].localname = tmp;
    open_files[fd].remotename = path;
    open_files[fd].modified = true;
    return 0;
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
