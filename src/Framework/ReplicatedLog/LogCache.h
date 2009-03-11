#ifndef LOGCACHE_H
#define LOGCACHE_H

#include "System/Containers/List.h"
#include "Framework/Paxos/PaxosMsg.h"

#define CACHE_SIZE		10000

class LogItem
{
public:
	ulong64					paxosID;
	//ByteArray<65*KB>		value;
	ByteString				value;
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
	char*				buffers[CACHE_SIZE];
	int					count;
	int					next;
	int					size;
};

#endif
