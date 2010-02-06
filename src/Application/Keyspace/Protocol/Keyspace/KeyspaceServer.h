#ifndef KEYSPACESERVER_H
#define KEYSPACESERVER_H

#include "../ProtocolServer.h"
#include "Framework/Transport/TCPServer.h"
#include "KeyspaceConn.h"

class KeyspaceDB;

class KeyspaceServer : public ProtocolServer,
public TCPServerT<KeyspaceServer, KeyspaceConn, KEYSPACE_BUF_SIZE>
{
public:
	void				Init(KeyspaceDB* kdb, int port);
	void				InitConn(KeyspaceConn* conn);
	void				DeleteConn(KeyspaceConn* conn);

private:
	KeyspaceDB*			kdb;
	List<KeyspaceConn*>	activeConns;
};

#endif
