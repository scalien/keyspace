#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "Framework/Transport/TCPConn.h"
#include "HttpRequest.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Database/KeyspaceService.h"

class HttpServer;

class HttpConn : public TCPConn<>
{
public:
	HttpConn();
	
	void			Init(HttpServer* server_);
	void			SetOnClose(Callable* callable);

	void			Print(const char* s);
	void			Response(int code, const char* buf,
					int len, bool close = true, const char* header = NULL);
	void			ResponseHeader(int code, bool close = true,
					const char* header = NULL);
	void			Flush(bool closeAfterSend = false);

	HttpServer*		GetServer() { return server; }

	// TCPConn interface
	virtual void	OnRead();
	virtual void	OnClose();
	virtual void	OnWrite();	

protected:
	Callable*		onCloseCallback;
	HttpServer*		server;
	HttpRequest		request;
	Endpoint		endpoint;
	bool			closeAfterSend;

	int				Parse(char* buf, int len);
	int				ProcessGetRequest();
	const char*		Status(int code);	
};

#endif
