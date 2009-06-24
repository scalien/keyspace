#ifndef KEYSPACESERVER_H
#define KEYSPACESERVER_H

#include "Framework/Transport/TCPServer.h"
#include "KeyspaceConn.h"

class KeyspaceDB;

class KeyspaceServer : public TCPServerT<KeyspaceServer, KeyspaceConn>
{
public:
	void			Init(KeyspaceDB* kdb, int port);
	void			InitConn(KeyspaceConn* conn);

private:
	KeyspaceDB*		kdb;
};

#endif
