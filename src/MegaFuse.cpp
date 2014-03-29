#include "MegaFuse.h"
#include "megafusemodel.h"

MegaFuse::MegaFuse(MegaFuseModel *m):model(m)
{
	
	
}


int MegaFuse::create(const char* path, mode_t mode, fuse_file_info* fi)
{
	return model->create(path,mode,fi);
}

int MegaFuse::getAttr(const char* path, stat* stbuf)
{
	return model->getAttr(path,stbuf);
}

int MegaFuse::mkdir(const char* path, mode_t mode)
{
	return model->mkdir(path,mode);
}

int MegaFuse::open(const char* path, fuse_file_info* fi)
{
	return model->open(path,fi);
}

int MegaFuse::read(const char* path, char* buf, size_t size, off_t offset, fuse_file_info* fi)
{
	return model->read(path,fi);
}

int MegaFuse::readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, fuse_file_info* fi)
{
	return model->readdir(path,buf,filler,offset,fi);
}


int MegaFuse::release(const char* path, fuse_file_info* fi)
{
	return model->release(path,fi);
}

int MegaFuse::rename(const char* src, const char* dst)
{
	return model->rename(src,dst);
}

int MegaFuse::write(const char* path, const char* buf, size_t size, off_t offset, fuse_file_info* fi)
{
	return model->write(path,bug,size,offset,fi);
}
