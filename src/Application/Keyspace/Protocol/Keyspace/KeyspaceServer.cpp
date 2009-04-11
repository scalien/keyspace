#include "KeyspaceServer.h"
#include "System/IO/IOProcessor.h"

#define KEYSPACE_PORT	7080
#define CONN_BACKLOG	10

void KeyspaceServer::Init(KeyspaceDB* kdb_)
{
	TCPServerT<KeyspaceServer, KeyspaceConn>::Init(KEYSPACE_PORT + kdb->GetNodeID(), CONN_BACKLOG);
	kdb = kdb_;
}

void KeyspaceServer::InitConn(KeyspaceConn* conn)
{
	conn->Init(kdb, this);
}
