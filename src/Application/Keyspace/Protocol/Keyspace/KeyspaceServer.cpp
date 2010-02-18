#include "KeyspaceServer.h"
#include "System/IO/IOProcessor.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"

#define KEYSPACE_PORT	7080
#define CONN_BACKLOG	10

KeyspaceServer::KeyspaceServer() :
onThruputTimeout(this, &KeyspaceServer::OnThruputTimeout),
thruputTimeout(1000, &onThruputTimeout)
{
}

void KeyspaceServer::Init(KeyspaceDB* kdb_, int port_)
{
	if (!TCPServerT<KeyspaceServer, KeyspaceConn, KEYSPACE_BUF_SIZE>
	::Init(port_, CONN_BACKLOG))
		STOP_FAIL("Cannot initialize KeyspaceServer", 1);
	kdb = kdb_;
	kdb->SetProtocolServer(this);
	OnThruputTimeout();
	bytesRead = 0;
}

void KeyspaceServer::InitConn(KeyspaceConn* conn)
{
	conn->Init(kdb, this);
	activeConns.Append(conn);
}

void KeyspaceServer::DeleteConn(KeyspaceConn* conn)
{
	TCPServerT<KeyspaceServer, KeyspaceConn, KEYSPACE_BUF_SIZE>
	::DeleteConn(conn);
	activeConns.Remove(conn);
}

void KeyspaceServer::OnThruputTimeout()
{
//	KeyspaceConn** it;
//
//	if (!kdb->IsReplicated())
//		return;
//
//	bytesRead = 0;
//	
//	for (it = activeConns.Head(); it != NULL; it = activeConns.Next(it))
//	{
//		(*it)->bytesRead = 0;
//		(*it)->Continue();
//	}
//		
//	EventLoop::Reset(&thruputTimeout);
}

void KeyspaceServer::OnDataRead(KeyspaceConn* conn, unsigned bytes)
{
//	uint64_t t;
//
//	if (!kdb->IsReplicated())
//		return;
//	
//	bytesRead += bytes;
//
//	t = MAX(2 * RLOG->GetLastRound_Thruput(), KEYSPACE_POOL_MIN_THRUPUT);
//
//	if (bytesRead > t &&
//	conn->bytesRead > KEYSPACE_CONN_MIN_THRUPUT)
//	{
//		Log_Trace("Throttling connection; bytesRead: %d", bytesRead);
//		conn->Stop();
//	}
}
