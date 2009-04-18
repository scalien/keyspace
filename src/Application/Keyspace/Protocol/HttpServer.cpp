#include "HttpServer.h"
#include "System/IO/IOProcessor.h"

#define CONN_BACKLOG	10

void HttpServer::Init(KeyspaceDB* kdb_, int port)
{
	if (!TCPServerT<HttpServer, HttpConn>::Init(port, CONN_BACKLOG))
		ASSERT_FAIL();
	kdb = kdb_;
}

void HttpServer::InitConn(HttpConn* conn)
{
	conn->Init(kdb, this);
}
