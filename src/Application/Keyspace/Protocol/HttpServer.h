#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "System/Containers/LinkedList.h"
#include "Framework/Transport/TCPServer.h"
#include "HttpConn.h"

class KeyspaceDB;

class HttpServer : public TCPServer
{
public:
	void			Init(KeyspaceDB* kdb);
	
	void			DeleteConn(HttpConn* conn);
	
private:
	typedef LinkedList<HttpConn, &HttpConn::node> HttpConnList;

	KeyspaceDB*		kdb;
	HttpConnList	conns;
	
	HttpConn*		GetConn();

	// TCPServer interface
	virtual void	OnConnect();
};

#endif
