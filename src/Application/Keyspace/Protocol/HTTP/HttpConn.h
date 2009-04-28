#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "System/Containers/LinkedList.h"
#include "Framework/Transport/TCPConn.h"
#include "HttpRequest.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Database/KeyspaceService.h"

class HttpServer;

class HttpConn : public TCPConn<>, public KeyspaceService
{
public:
	HttpConn();
	
	void			Init(KeyspaceDB* kdb_, HttpServer* server_);
	void			OnComplete(KeyspaceOp* op, bool status, bool final);

private:	
	HttpServer*		server;
	HttpRequest		request;
	bool			headerSent;
	bool			closeAfterSend;

	// TCPConn interface
	virtual void	OnRead();
	virtual void	OnClose();
	virtual void	OnWrite();
	
	int				Parse(char* buf, int len);
	int				ProcessGetRequest();
	const char*		Status(int code);
	void			Response(int code, const char* buf, int len, bool close = true, const char* header = NULL);
	void			ResponseHeader(int code, bool close = true, const char* header = NULL);
};

#endif
