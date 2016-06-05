#include <fuse.h>
#include <thread>
#include "EventsHandler.h"

#define MEGAFUSE_BUFF_SIZE	1024

class MegaFuseModel;
class MegaFuse
{
private:
	MegaFuseModel *model;
	EventsHandler eh;
	byte pwkey[SymmCipher::KEYLENGTH];
	std::mutex engine_mutex;
	bool running;
	static void event_loop(MegaFuse* megaFuse);
	static void maintenance(MegaFuse* megaFuse);
	std::thread event_loop_thread;
	public:
	MegaFuse();
	~MegaFuse();
	bool login();
	bool xattrLoaded;
	bool symlinkLoaded;
	bool needSerializeSymlink;
	bool needSerializeXattr;

	struct Xattr_value
	{
		char *path;
		char *name;
		char *value;
		size_t size;
	};

	map<std::string, map<std::string, struct Xattr_value*>> xattr_list;

	typedef map<std::string, struct Xattr_value*> Map_inside;
	typedef map<std::string, Map_inside> Map_outside;

	map<std::string, mode_t> modelist;
	
	int open(const char *path, struct fuse_file_info *fi);
	int readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi);
	int getAttr(const char *path, struct stat *stbuf);
	int release(const char *path, struct fuse_file_info *fi);
	int releaseNoThumb(const char *path, struct fuse_file_info *fi);
	int mkdir(const char * path, mode_t mode);
	int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
	int create(const char *path, mode_t mode, struct fuse_file_info * fi);
	int write(const char * path, const void *buf, size_t size, off_t offset, struct fuse_file_info * fi);
	int write(const char * path, const char *buf, size_t size, off_t offset, struct fuse_file_info * fi);
	int rename(const char * src, const char *dst);
	int unlink(const char*);
	int truncate(const char *a,off_t o);

	int setxattr(const char *path, const char *name, const char *value, size_t size, int flags);
	int raw_setxattr(const char *path, const char *name, const char *value, size_t size, int flags);
	int getxattr(const char *path, const char *name, char *value, size_t size);
	int symlink(const char *path1, const char *path2);
	int readlink(const char *path, char *buf, size_t bufsiz);
	int serializeXattr();
	uint32_t addChunk(void *binbuff, uint32_t *buffsize, char *data, uint32_t data_size);
	int unserializeXattr();
	int serializeSymlink();
	int unserializeSymlink();
	int stringToFile(const char *path, std::string tmpbuf, size_t bufsize, fuse_file_info *fi);




	
	
};
