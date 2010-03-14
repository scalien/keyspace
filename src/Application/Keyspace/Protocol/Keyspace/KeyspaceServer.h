#ifndef KEYSPACESERVER_H
#define KEYSPACESERVER_H

#include "../ProtocolServer.h"
#include "System/Events/Timer.h"
#include "Framework/Transport/TCPServer.h"
#include "KeyspaceConn.h"

#define KEYSPACE_POOL_MIN_THRUPUT		250*KB
#define KEYSPACE_CONN_MIN_THRUPUT		10*KB

class KeyspaceDB;

class KeyspaceServer : public ProtocolServer,
public TCPServerT<KeyspaceServer, KeyspaceConn, KEYSPACE_BUF_SIZE>
{
typedef MFunc<KeyspaceServer>	Func;

public:
	KeyspaceServer();
	
	void				Init(KeyspaceDB* kdb, int port);
	void				Shutdown();
	
	void				InitConn(KeyspaceConn* conn);
	void				DeleteConn(KeyspaceConn* conn);
	void				OnThruputTimeout();
	void				OnDataRead(KeyspaceConn* conn, unsigned bytes);

	uint64_t			bytesRead;

private:
	KeyspaceDB*			kdb;
	List<KeyspaceConn*>	activeConns;
	Func				onThruputTimeout;
	CdownTimer			thruputTimeout;
};

#endif
