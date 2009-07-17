#include "MemcacheServer.h"

void MemcacheServer::Init(KeyspaceDB* kdb_)
{
	if (!TCPServerT<MemcacheServer, MemcacheConn>::Init(MEMCACHE_PORT, 10))
		ASSERT_FAIL();
	kdb = kdb_;
}

void MemcacheServer::InitConn(MemcacheConn* conn)
{
	conn->Init(this, kdb);
}

