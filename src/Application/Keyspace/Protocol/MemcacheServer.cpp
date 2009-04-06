#include "MemcacheServer.h"

void MemcacheServer::Init(KeyspaceDB* kdb_)
{
	Log_Trace();

	TCPServerT<MemcacheServer, MemcacheConn>::Init(MEMCACHE_PORT, 10);

	kdb = kdb_;
}

void MemcacheServer::InitConn(MemcacheConn* conn)
{
	Log_Trace();

	conn->Init(this, kdb);
}

