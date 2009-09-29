#include "KeyspaceServer.h"
#include "System/IO/IOProcessor.h"

#define KEYSPACE_PORT	7080
#define CONN_BACKLOG	0

void KeyspaceServer::Init(KeyspaceDB* kdb_, int port_)
{
	if (!TCPServerT<KeyspaceServer, KeyspaceConn>::Init(port_, CONN_BACKLOG))
		STOP_FAIL("Cannot initialize KeyspaceServer", 1);
	kdb = kdb_;
	kdb->SetProtocolServer(this);
}

void KeyspaceServer::InitConn(KeyspaceConn* conn)
{
	conn->Init(kdb, this);
	activeConns.Append(conn);
}

void KeyspaceServer::DeleteConn(KeyspaceConn* conn)
{
	TCPServerT<KeyspaceServer, KeyspaceConn>::DeleteConn(conn);
	activeConns.Remove(conn);
}

void KeyspaceServer::Stop()
{
	Log_Trace();
	
	KeyspaceConn** it;
		
	IOProcessor::Remove(&tcpread);
	
	for (it = activeConns.Head(); it != NULL; it = activeConns.Next(it))
	{
		(*it)->Stop();
		if (tcpread.active)
			break; // user called Continue(), stop looping
	}
}

void KeyspaceServer::Continue()
{
	Log_Trace();

	KeyspaceConn** it;
	
	IOProcessor::Add(&tcpread);
	
	for (it = activeConns.Head(); it != NULL; it = activeConns.Next(it))
	{
		(*it)->Continue();
		if (!tcpread.active)
			break; // user called Stop(), stop looping
	}
}
