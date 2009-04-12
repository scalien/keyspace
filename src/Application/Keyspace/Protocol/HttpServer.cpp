#include "HttpServer.h"
#include "System/IO/IOProcessor.h"

#define CONN_BACKLOG	10

void HttpServer::Init(KeyspaceDB* kdb_, int port)
{
	TCPServerT<HttpServer, HttpConn>::Init(port, CONN_BACKLOG);
	kdb = kdb_;
}

void HttpServer::InitConn(HttpConn* conn)
{
	conn->Init(kdb, this);
}
