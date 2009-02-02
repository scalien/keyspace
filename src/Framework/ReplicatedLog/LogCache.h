#ifndef LOGCACHE_H
#define LOGCACHE_H

#include "System/Containers/List.h"
#include "Framework/Paxos/PaxosMsg.h"

#define CACHE_SIZE		10*1000

class Entry
{
public:
	ulong64					paxosID;
	ByteArray<VALUE_SIZE>	value;
};

class LogCache
{
public:
	LogCache()			{ count = 0; next = 0; size = SIZE(array); }

	Entry*				Last();

	bool				Push(ulong64 paxosID, ByteString value);
	Entry*				Get(ulong64 paxosID);

private:
	Entry				array[CACHE_SIZE];
	int					count;
	int					next;
	int					size;
};

#endif