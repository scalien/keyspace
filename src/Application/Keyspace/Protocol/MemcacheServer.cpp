#include "MemcacheServer.h"

void MemcacheServer::Init(KeyspaceDB* kdb_)
{
	Log_Trace();

	TCPServer::Init(IOProcessor::Get(), MEMCACHE_PORT);

	kdb = kdb_;
}

void MemcacheServer::OnDisconnect(MemcacheConn* conn)
{
	Log_Trace();

	conns.Remove(conn);
}

void MemcacheServer::OnConnect()
{
	Log_Trace();

	// FIXME
	IOProcessor* ioproc = IOProcessor::Get();
	
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
