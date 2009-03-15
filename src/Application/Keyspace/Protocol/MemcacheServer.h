#ifndef MEMCACHE_SERVER_H
#define MEMCACHE_SERVER_H

#include "System/IO/IOOperation.h"
#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Containers/List.h"
#include "Framework/Transport/TCPServer.h"
#include "MemcacheConn.h"

#define MEMCACHE_PORT 11111

class KeyspaceDB;

class MemcacheServer : public TCPServer 
{
public:
	void					Init(KeyspaceDB* kdb);
	void					OnDisconnect(MemcacheConn* conn);
private:	
	List<MemcacheConn *>	conns;
	KeyspaceDB*				kdb;
	virtual void			OnConnect();
};

#endif
