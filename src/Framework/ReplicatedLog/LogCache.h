#ifndef LOGCACHE_H
#define LOGCACHE_H

#include "System/Containers/List.h"
#include "Framework/Paxos/PaxosMsg.h"

#define CACHE_SIZE		10*1000
#define MAX_MEM_USE		256*MB

class LogCache
{
public:
						LogCache();
						~LogCache();

	bool				Push(uint64_t paxosID, ByteString value);
	bool				Get(uint64_t paxosID, ByteString& value);

private:
	ByteBuffer			logItems[CACHE_SIZE];
	int					count;
	int					next;
	int					size;
	unsigned long		allocated;
	uint64_t				paxosID;
};

#endif
