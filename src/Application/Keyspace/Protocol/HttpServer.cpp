#include "HttpServer.h"
#include "System/IO/IOProcessor.h"

#define HTTP_PORT		8080
#define CONN_BACKLOG	10

void HttpServer::Init(KeyspaceDB* kdb_)
{
	kdb = kdb_;
	TCPServer::Init(HTTP_PORT + kdb->GetNodeID());
}

void HttpServer::DeleteConn(HttpConn* conn)
{
	if (conns.Size() >= CONN_BACKLOG)
		delete conn;
	else
		conns.Add(*conn);
}

HttpConn* HttpServer::GetConn()
{
	HttpConn* conn;
	
	if (conns.Size() > 0)
	{
		conn = conns.Head();
		conns.Remove(conn);
		return conn;
	}
	
	return new HttpConn;
}

void HttpServer::OnConnect()
{
	IOProcessor* ioproc = IOProcessor::Get();
	HttpConn* conn = GetConn();
	if (listener.Accept(&(conn->GetSocket())))
	{
		conn->GetSocket().SetNonblocking();
		conn->Init(kdb, this);
	}
	else
	{
		Log_Message("Accept() failed");
		DeleteConn(conn);
	}
	
	ioproc->Add(&tcpread);
}
