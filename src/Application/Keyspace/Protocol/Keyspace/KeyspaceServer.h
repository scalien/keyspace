#ifndef KEYSPACESERVER_H
#define KEYSPACESERVER_H

#include "System/Containers/LinkedList.h"
#include "Framework/Transport/TCPServer.h"
#include "KeyspaceConn.h"

class KeyspaceDB;

class KeyspaceServer : public TCPServerT<KeyspaceServer, KeyspaceConn>
{
public:
	void			Init(KeyspaceDB* kdb);
	void			InitConn(KeyspaceConn* conn);

private:
	KeyspaceDB*		kdb;
};

#endif
