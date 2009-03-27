#ifndef LOGCACHE_H
#define LOGCACHE_H

#include "System/Containers/List.h"
#include "Framework/Paxos/PaxosMsg.h"

#define CACHE_SIZE		10*1000
#define MAX_MEM_USE		256*MB

class LogItem
{
public:
	ulong64					paxosID;
	ByteBuffer				value;
};

class LogCache
{
public:
						LogCache();
						~LogCache();

	LogItem*			Last();

	bool				Push(ulong64 paxosID, ByteString value);
	LogItem*			Get(ulong64 paxosID);

private:
	LogItem				logItems[CACHE_SIZE];
	int					count;
	int					next;
	int					size;
	unsigned long		allocated;
};

#endif
