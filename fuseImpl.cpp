#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "megaclient.h"
#include "megafuse.h"
static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";

static MegaFuse* megaFuse;


struct file_info
{
    std::string remotename;
    std::string localname;
    bool modified;
    file_info(): modified(false){};
};

std::map<uint64_t,file_info> open_files;

extern "C" int hello_write(const char *, const char *buf, size_t size, off_t offset, struct fuse_file_info * fi)
{
    int s = pwrite(fi->fh,buf,size,offset);
    open_files[fi->fh].modified = true;
    return s;
}

extern "C" int hello_open(const char *path, struct fuse_file_info *fi)
{
    char *tmp = tmpnam(NULL);
    auto result = megaFuse->download(std::string(path),tmp);

    printf("fuse: apro %s\n",tmp);
	if (!result )
		return -ENOENT;
    chmod(tmp, S_IRUSR | S_IWUSR);
    int fd = open (tmp,O_RDWR);

    if (fd < 0)
        return -EIO;

    fi->fh = fd;
    open_files[fd].localname = tmp;
    open_files[fd].remotename = path;
	return 0;
}

extern "C" int hello_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	int s = pread(fi->fh,buf,size,offset);

	return s;
}



extern "C" int hello_release(const char *path, struct fuse_file_info *fi)
{
    printf("fuse: release chiamato\n");
    close(fi->fh);
    if(open_files[fi->fh].modified)
        megaFuse->upload(open_files[fi->fh].localname,open_files[fi->fh].remotename);

    unlink(open_files[fi->fh].localname.c_str());
    open_files.erase(fi->fh);
    return 0;
}

extern "C" int hello_unlink(const char *path)
{
    return megaFuse->unlink(path);
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
    }
	return res;
}

extern "C" int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	//if (strcmp(path, "/") != 0)
		//return -ENOENT;

    auto l = megaFuse->ls(std::string(path));
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	for(auto t = l.cbegin(); t!= l.cend(); t++)
        filler(buf, t->c_str(), NULL, 0);

	return 0;
}

extern "C" int megafuse_main(int argc,char**argv);
int megafuse_mainpp(int argc,char**argv,MegaFuse* mf)
{
    megaFuse = mf;
    return megafuse_main(argc, argv);

}
