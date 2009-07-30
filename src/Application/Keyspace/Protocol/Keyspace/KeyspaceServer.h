#ifndef KEYSPACESERVER_H
#define KEYSPACESERVER_H

#include "../ProtocolServer.h"
#include "Framework/Transport/TCPServer.h"
#include "KeyspaceConn.h"

class KeyspaceDB;

class KeyspaceServer : public ProtocolServer,
public TCPServerT<KeyspaceServer, KeyspaceConn>
{
public:
	void				Init(KeyspaceDB* kdb, int port);
	void				InitConn(KeyspaceConn* conn);
	void				DeleteConn(KeyspaceConn* conn);
	
	virtual void		Stop();
	virtual void		Continue();

private:
	KeyspaceDB*			kdb;
	List<KeyspaceConn*>	activeConns;
};

#endif
