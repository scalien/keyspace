#include "KeyspaceServer.h"
#include "System/IO/IOProcessor.h"

#define KEYSPACE_PORT	7080
#define CONN_BACKLOG	10

void KeyspaceServer::Init(KeyspaceDB* kdb_, int port_)
{
	if (!TCPServerT<KeyspaceServer, KeyspaceConn>::Init(port_, CONN_BACKLOG))
		STOP_FAIL("Cannot initialize KeyspaceServer", 1);
	kdb = kdb_;
}

void KeyspaceServer::InitConn(KeyspaceConn* conn)
{
	conn->Init(kdb, this);
}
