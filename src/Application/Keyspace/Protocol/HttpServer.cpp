#include "HttpServer.h"
#include "System/IO/IOProcessor.h"

#define HTTP_PORT 8080

void HttpServer::Init(KeyspaceDB* kdb_)
{
	TCPServer::Init(IOProcessor::New(), HTTP_PORT);
	kdb = kdb_;
}

void HttpServer::OnConnect()
{
	IOProcessor* ioproc = IOProcessor::New();
	HttpConn* conn = new HttpConn;
	if (listener.Accept(&(conn->GetSocket())))
	{
		conn->GetSocket().SetNonblocking();
		conn->Init(ioproc, kdb, this);
	}
	else
	{
		Log_Message("Accept() failed");
		delete conn;
	}
	
	ioproc->Add(&tcpread);	
	
}
