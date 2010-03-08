#ifndef LOGCACHE_H
#define LOGCACHE_H

#include "System/Buffer.h"
#include "Framework/Database/Table.h"
#include "Framework/Paxos/PaxosConsts.h"

#define LOGCACHE_DEFAULT_SIZE	(100*1000)	// # of Paxos rounds cached in db

class LogCache
{
public:
	LogCache();
	~LogCache();

	bool			Init(uint64_t paxosID);
	bool			Push(uint64_t paxosID, ByteString value, bool commit);
	bool			Get(uint64_t paxosID, ByteString& value);

private:
	void			DeleteOldRounds(uint64_t paxosID);

	Table*			table;
	ByteArray<RLOG_SIZE> value;
	uint64_t		logCacheSize;
};

#endif
