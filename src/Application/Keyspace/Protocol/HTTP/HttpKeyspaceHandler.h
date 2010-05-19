#ifndef KEYSPACE_HTTP_HANDLER_H
#define KEYSPACE_HTTP_HANDLER_H

#include "HttpServer.h"

class HttpConn;

class HttpKeyspaceHandler : public HttpHandler
{
public:
	HttpKeyspaceHandler(KeyspaceDB* kdb_);
	
	// HttpHandler interface
	virtual bool	HandleRequest(HttpConn* conn, const HttpRequest& request);

private:
	KeyspaceDB*		kdb;
};


#endif
