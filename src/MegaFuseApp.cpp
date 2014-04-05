#include "mega.h"

#include "megacrypto.h"
#include "megaclient.h"
#include "megafusemodel.h"

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

MegaFuseApp::MegaFuseApp(MegaFuseModel* m):model(m)
{

}

MegaFuseApp::~MegaFuseApp()
{

}

void MegaFuseApp::login_result(error e)
{
	return model->login_result(e);
}

void MegaFuseApp::nodes_updated(Node** a, int b)
{
	return model->nodes_updated(a,b);
}

void MegaFuseApp::putfa_result(handle h, fatype f , error e)
{
	return model->putfa_result (h,f,e);
}

void MegaFuseApp::putnodes_result(error a , targettype b , NewNode* c)
{
	return model->putnodes_result(a,b,c);
}

void MegaFuseApp::topen_result(int td, error e)
{
	return model->topen_result(td,e);
}

void MegaFuseApp::topen_result(int td, string* filename, const char* fa, int pfa)
{
	return model->topen_result(td,filename,fa,pfa);
}

void MegaFuseApp::transfer_complete(int a, chunkmac_map* b, const char* c)
{
	return model->transfer_complete(a,b,c);
}

void MegaFuseApp::transfer_complete(int td, handle uploadhandle, const byte* uploadtoken, const byte* filekey, SymmCipher* key)
{
	return model->transfer_complete(td,uploadhandle,uploadtoken,filekey,key);
}

void MegaFuseApp::transfer_failed(int a, error e)
{
	return model->transfer_failed(a,e);
}

void MegaFuseApp::transfer_failed(int a, string& b , error c)
{
	return model->transfer_failed(a,b,c);
}

void MegaFuseApp::transfer_update(int a, m_off_t b , m_off_t c , dstime d)
{
	return model->transfer_update(a,b,c,d);
}

void MegaFuseApp::unlink_result(handle a, error b)
{
	return model->unlink_result(a,b);
}

void MegaFuseApp::users_updated(User** u, int count)
{
	return model->users_updated(u,count);
}
