#include "LogCache.h"
#include "ReplicatedLog.h"
#include "Framework/Database/Transaction.h"
#include "stdint.h"

LogCache::LogCache()
{
}

LogCache::~LogCache()
{
}

bool LogCache::Init()
{
	table = database.GetTable("keyspace");
	
	return true;
}

bool LogCache::Push(uint64_t paxosID, ByteString value)
{
	ByteArray<128> buf;
	Transaction* transaction;
	
	transaction = RLOG->GetTransaction();
	
	if (!transaction->IsActive())
		transaction->Begin();
	
	buf.Writef("@@round:%U", paxosID);	
	table->Set(transaction, buf, value);
	
	// delete old
	paxosID -= LOGCACHE_SIZE;
	if (paxosID >= 0)
	{
		buf.Writef("@@round:%U", paxosID);	
		table->Delete(transaction, buf);
	}
	
	return true;
}

bool LogCache::Get(uint64_t paxosID, ByteString& value_)
{
	ByteArray<128> buf;

	buf.Writef("@@round:%U", paxosID);
	if (table->Get(NULL, buf, value))
	{
		value_ = value;
		return true;
	}
	return false;
}
