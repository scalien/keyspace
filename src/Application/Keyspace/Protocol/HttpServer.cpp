#include "HttpServer.h"
#include "System/IO/IOProcessor.h"

#define HTTP_PORT		8080
#define CONN_BACKLOG	10

void HttpServer::Init(KeyspaceDB* kdb_)
{
	TCPServerT<HttpServer, HttpConn>::Init(HTTP_PORT + kdb->GetNodeID(), CONN_BACKLOG);
	kdb = kdb_;
}

void HttpServer::InitConn(HttpConn* conn)
{
	conn->Init(kdb, this);
}
