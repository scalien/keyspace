#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "Framework/Transport/TCPConn.h"
#include "HttpRequest.h"
#include "../Database/KeyspaceDB.h"

class HttpServer;

class HttpConn : public TCPConn<>, public KeyspaceClient
{
public:
	HttpConn();
	
	void			Init(IOProcessor* ioproc_, KeyspaceDB* kdb_, HttpServer* server_);
	void			OnComplete(KeyspaceOp* op, bool status);
private:
	
	HttpServer*		server;
	KeyspaceDB*		kdb;
	int				contentLength;
	int				contentStart;
	int				offs;
	char*			buffer;
	HttpRequest		request;

	// TCPConn interface
	virtual void	OnRead();
	virtual void	OnClose();
	
	int				Parse(char* buf, int len);
	int				ProcessGetRequest();
	const char*		Status(int code);
	void			Response(int code, char* buf, int len, bool close = true, char* header = NULL);
};

#endif
