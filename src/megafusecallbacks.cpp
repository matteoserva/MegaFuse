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



std::string last_thumbnail;
SymmCipher last_key;

void createthumbnail(const char* filename, unsigned size, string* result);


void MegaFuse::transfer_complete(int td, handle ulhandle, const byte* ultoken, const byte* filekey, SymmCipher* key)
{

    //DemoApp::transfer_complete(td,uploadhandle,uploadtoken,filekey,key);
    printf("upload riuscito\n");
    FilePut* putf = (FilePut*)client->gettransfer((transfer_list*)&client->putq,td);

	if (putf)
	{
		if (!putf->targetuser.size() && !client->nodebyhandle(putf->target))
		{
			cout << "Upload target folder inaccessible, using /" << endl;
			putf->target = client->rootnodes[0];
		}

		NewNode* newnode = new NewNode[1];

		// build new node
		newnode->source = NEW_UPLOAD;

		// upload handle required to retrieve/include pending file attributes
		newnode->uploadhandle = ulhandle;

		// reference to uploaded file
		memcpy(newnode->uploadtoken,ultoken,sizeof newnode->uploadtoken);

		// file's crypto key
		newnode->nodekey.assign((char*)filekey,Node::FILENODEKEYLENGTH);
		newnode->mtime = newnode->ctime = time(NULL);
		newnode->type = FILENODE;
		newnode->parenthandle = UNDEF;

		AttrMap attrs;

		MegaClient::unescapefilename(&putf->filename);

		attrs.map['n'] = putf->newname.size() ? putf->newname : putf->filename;

		attrs.getjson(&putf->filename);

		client->makeattr(key,&newnode->attrstring,putf->filename.c_str());

        {

            Node *p = client->nodebyhandle(putf->target);
            for (node_list::iterator it = p->children.begin(); it != p->children.end(); it++)
            {
                if (!strcmp(putf->newname.c_str(),(*it)->displayname()))
                {
                    client->unlink(*it);
                    break;
                }
            }
        }

		if (putf->targetuser.size())
		{
			cout << "Attempting to drop file into user " << putf->targetuser << "'s inbox..." << endl;
			client->putnodes(putf->targetuser.c_str(),newnode,1);
		}
		else client->putnodes(putf->target,newnode,1);



        auto it = file_cache.end();
            for(auto i = file_cache.begin();i!=file_cache.end();i++)
                if(i->second.localname == putf->filename)
                    it = i;
        it->second.status = file_cache_row::AVAILABLE;
        it->second.td = -1;
        it->second.modified = false;



		delete putf;
	}
	else cout << "(Canceled transfer, ignoring)" << endl;

	client->tclose(td);


    //upload_lock.unlock();
}

void MegaFuse::putfa_result(handle, fatype, error e)
{
    printf("putfa ricevuto\n\n");
    if(e)
        printf("errore putfa\n");



}
void MegaFuse::topen_result(int td, error e)
{

	printf("topen fallito!\n");
    last_error = e;
	client->tclose(td);
    {
        std::lock_guard<std::mutex> lk(cvm);
        opend_ret = -1;
    }
    cv.notify_one();
	//open_lock.unlock();
}

void MegaFuse::unlink_result(handle h, error e)
{

	printf("unlink eseguito\n");
    {
        std::lock_guard<std::mutex> lk(cvm);
        unlink_ret  = (e)?-1:1;
    }
    cv.notify_one();
	//unlink_lock.unlock();
}
// topen() succeeded (download only)
void MegaFuse::topen_result(int td, string* filename, const char* fa, int pfa)
{
    last_error = API_OK;
    result = td;
	printf("topen riuscito\n");



    std::string remotename = "";
    std::string tmp;
    for(auto it = file_cache.cbegin();it!=file_cache.cend();++it)
        if(it->second.status == file_cache_row::WAIT_D_TOPEN && it->second.td == td)
            {
                remotename = it->first;
                tmp = it->second.localname;
            }
        chmod(tmp.c_str(),S_IWUSR|S_IRUSR);
    client->dlopen(td,tmp.c_str());
    printf("file: %s ora in stato DOWNLOADING\n",remotename.c_str());
    file_cache[remotename].status = file_cache_row::DOWNLOADING;
    file_cache[remotename].localname = tmp;
    file_cache[remotename].available_bytes = 0;
    {
        std::lock_guard<std::mutex> lk(cvm);
        opend_ret = 1;
    }
    cv.notify_all();
}

void MegaFuse::transfer_failed(int td, string& filename, error e)
{
    printf("download fallito: %d\n",e);
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
    printf("download complete\n");
    client->tclose(it->second.td);
    it->second.td = -1;

    if(it->second.startOffset == 0)
    {
        std::string remotename = it->first;
        {
            std::lock_guard<std::mutex> lk(cvm);
            it->second.status = file_cache_row::AVAILABLE;
        }
        cv.notify_all();

        printf("download riuscito per %s,%d\n",remotename.c_str(),td);
        last_error=API_OK;
    }
    else
    {
			printf("----------------download reissued for %s\n",it->first.c_str());
            Node*n = nodeByPath(it->first);
            int td = client->topen(n->nodehandle, NULL, 0,-1, 1);
            if(td < 0)
                return;
            it->second.td = td;
            it->second.startOffset = 0;
            it->second.status = file_cache_row::WAIT_D_TOPEN;
            it->second.available_bytes=0;

    }


    //download_lock.unlock();
}

void MegaFuse::transfer_update(int td, m_off_t bytes, m_off_t size, dstime starttime)
{

	static time_t last = time(NULL);

    std::string remotename = "";

    auto it = findCacheByTransfer(td,file_cache_row::DOWNLOADING );
    if(it == file_cache.end())
    {
		cout << '\r'<<"UPLOAD TD " << td << ": Update: " << bytes/1024 << " KB of " << size/1024 << " KB, " << bytes*10/(1024*(client->httpio->ds-starttime)+1) << " KB/s" ;

        fflush(stdout);
        return;
    }

    if(time(NULL) - last >= 1)
        {
            cout << remotename << td << ": Update: " << bytes/1024 << " KB of " << size/1024 << " KB, " << bytes*10/(1024*(client->httpio->ds-starttime)+1) << " KB/s" << endl;
        cout << "scaricato fino al byte " <<(it->second.startOffset+bytes) << " di: "<<size<<endl;
        last = time(NULL);
        }

    {
        std::lock_guard<std::mutex> lk(cvm);
        struct stat st;
        stat(it->second.localname.c_str(),&st);

        it->second.available_bytes = st.st_size;
    }
    cv.notify_all();
	std::this_thread::yield();

    //WORKAROUND
    if(it->second.startOffset && (it->second.startOffset+bytes)>size && it->second.available_bytes>= it->second.size)
       transfer_complete(td,NULL,NULL);
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
