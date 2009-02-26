#include "MemcacheServer.h"

void MemcacheServer::Init(KeyspaceDB* kdb_)
{
	kdb = kdb_;
}

void MemcacheServer::OnDisconnect(MemcacheConn* conn)
{
	conns.Remove(conn);
}

void MemcacheServer::OnConnect()
{
	// FIXME
	IOProcessor* ioproc = IOProcessor::New();
	
	MemcacheConn *conn = new MemcacheConn;
	if (listener.Accept(&(conn->socket)))
	{
		conn->socket.SetNonblocking();
		conn->Init(ioproc, kdb, this);
		conns.Add(conn);
	}
	else
	{
		Log_Message("Accept() failed");
		delete conn;
	}
	
	ioproc->Add(&tcpread);	
}