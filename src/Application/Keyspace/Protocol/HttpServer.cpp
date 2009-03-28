#include "HttpServer.h"
#include "System/IO/IOProcessor.h"

#define HTTP_PORT 8080

void HttpServer::Init(KeyspaceDB* kdb_)
{
	kdb = kdb_;
	TCPServer::Init(HTTP_PORT + kdb->GetNodeID());
}

void HttpServer::OnConnect()
{
	IOProcessor* ioproc = IOProcessor::Get();
	HttpConn* conn = new HttpConn;
	if (listener.Accept(&(conn->GetSocket())))
	{
		conn->GetSocket().SetNonblocking();
		conn->Init(kdb, this);
	}
	else
	{
		Log_Message("Accept() failed");
		delete conn;
	}
	
	ioproc->Add(&tcpread);	
	
}
