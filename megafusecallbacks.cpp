#include "megaclient.h"
#include "megafuse.h"

void MegaFuse::putnodes_result(error e, targettype, NewNode* nn)
{
    delete[] nn;

    {
        std::lock_guard<std::mutex> lk(cvm);
        putnodes_ret  = (e)?-1:1;
    }
    cv.notify_one();
}

void MegaFuse::transfer_failed(int td,  error e)
{
    printf("upload fallito\n");
    last_error = e;
  //  upload_lock.unlock();
}
void MegaFuse::transfer_complete(int td, handle uploadhandle, const byte* uploadtoken, const byte* filekey, SymmCipher* key)
{
    //DemoApp::transfer_complete(td,uploadhandle,uploadtoken,filekey,key);
    printf("upload riuscito");
    client->tclose(td);
    //upload_lock.unlock();
}


void MegaFuse::topen_result(int td, error e)
{

	printf("topen fallito\n");
    last_error = e;
	client->tclose(td);
	//open_lock.unlock();
}

void MegaFuse::unlink_result(handle h, error e)
{

	printf("unlink eseguito\n");
    last_error = e;
	//unlink_lock.unlock();
}
// topen() succeeded (download only)
void MegaFuse::topen_result(int td, string* filename, const char* fa, int pfa)
{
    last_error = API_OK;
    result = td;
	printf("topen riuscito\n");
	char *tmp = tmpnam(NULL);
    client->dlopen(td,tmp);

    std::string remotename = "";

    for(auto it = file_cache.cbegin();it!=file_cache.cend();++it)
        if(it->second.status == file_cache_row::WAIT_D_TOPEN && it->second.td == td)
            remotename = it->first;
    printf("file: %s ora in stato DOWNLOADING\n",remotename.c_str());
    file_cache[remotename].status = file_cache_row::DOWNLOADING;
    file_cache[remotename].localname = tmp;
    {
        std::lock_guard<std::mutex> lk(cvm);
        opend_ret = 1;
    }
    cv.notify_one();
}

void MegaFuse::transfer_failed(int td, string& filename, error e)
{
    printf("download fallito\n");
    //DemoApp::transfer_failed(td,filename,e);
    last_error=e;
    //download_lock.unlock();
}

std::map <std::string,file_cache_row>::iterator MegaFuse::findCacheByTransfer(int td, file_cache_row::CacheStatus status)
{
    for(auto it = file_cache.begin();it!=file_cache.end();++it)
        if(it->second.status == status && it->second.td == td)
            return it;
    return file_cache.end();
}

void MegaFuse::transfer_complete(int td, chunkmac_map* macs, const char* fn)
{
    auto it = findCacheByTransfer(td,file_cache_row::DOWNLOADING );
    std::string remotename = it->first;
    {
        std::lock_guard<std::mutex> lk(cvm);
        it->second.status = file_cache_row::AVAILABLE;
    }
    cv.notify_all();

    printf("download riuscito per %s,%d\n",remotename.c_str(),td);
    client->tclose(td);
    last_error=API_OK;
    //download_lock.unlock();
}

void MegaFuse::transfer_update(int td, m_off_t bytes, m_off_t size, dstime starttime)
{
	static time_t last = time(NULL);
	if(time(NULL) - last < 1)
        return;
    std::string remotename = "";

    auto it = findCacheByTransfer(td,file_cache_row::DOWNLOADING );

	cout << remotename << td << ": Update: " << bytes/1024 << " KB of " << size/1024 << " KB, " << bytes*10/(1024*(client->httpio->ds-starttime)+1) << " KB/s" << endl;

	last = time(NULL);
    {
        std::lock_guard<std::mutex> lk(cvm);
        struct stat st;
        stat(it->second.localname.c_str(),&st);

        it->second.available_bytes = st.st_size;
    }
    cv.notify_all();
}

void MegaFuse::login_result(error e)
{
    printf("risultato arrivato\n");
    {
        std::lock_guard<std::mutex> lk(cvm);
        login_ret = (e)? -1:1;
    }
    cv.notify_one();
    if(!e)
    client->fetchnodes();
    printf("fine login_result\n");
}
