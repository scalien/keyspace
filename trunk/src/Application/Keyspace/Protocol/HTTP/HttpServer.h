#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "System/Containers/LinkedList.h"
#include "Framework/Transport/TCPServer.h"
#include "HttpConn.h"

class KeyspaceDB;

class HttpServer : public TCPServerT<HttpServer, HttpConn>
{
public:
	void			Init(KeyspaceDB* kdb, int port);
	void			InitConn(HttpConn* conn);

private:
	KeyspaceDB*		kdb;
};

#endif
