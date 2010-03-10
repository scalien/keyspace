#include "LogCache.h"
#include "ReplicatedLog.h"
#include "System/Config.h"
#include "Framework/Database/Transaction.h"
#include "System/Platform.h"


static void WriteRoundID(ByteString& bs, uint64_t paxosID)
{
	// 2^64 is at most 19 digits
	bs.length = snprintf(bs.buffer, bs.size, "@@pround:%020" PRIu64 "", paxosID);
}

void LogCache::DeleteOldRounds(uint64_t paxosID)
{
	Cursor			cursor;
	ByteArray<128>	buf;
	DynArray<128>	key;
	DynArray<128>	value;
	Transaction*	transaction;
	
	transaction = RLOG->GetTransaction();
	
	transaction->Begin();
	
	WriteRoundID(buf, paxosID);
	table->Iterate(transaction, cursor);
	if (!cursor.Start(buf))
	{
		cursor.Close();
		transaction->Rollback();
		return;
	}
		
	while (cursor.Prev(key, value))
	{
		if (key.length > sizeof("@@pround:") - 1 &&
		strncmp(key.buffer, "@@pround:", sizeof("@@pround:") - 1) == 0)
			cursor.Delete();
		else
			break;
	}
	
	cursor.Close();

	transaction->Commit();
}

LogCache::LogCache()
{
}

LogCache::~LogCache()
{
}

bool LogCache::Init(uint64_t paxosID)
{
	table = database.GetTable("keyspace");
	
	logCacheSize = Config::GetIntValue("rlog.cacheSize", LOGCACHE_DEFAULT_SIZE);

	if ((int64_t)(paxosID - logCacheSize) >= 0)
		DeleteOldRounds(paxosID - logCacheSize);

	return true;
}

bool LogCache::Push(uint64_t paxosID, ByteString value, bool commit)
{
	ByteArray<128> buf;
	Transaction* transaction;
	
	Log_Trace("Storing paxosID %" PRIu64 " with length %d", paxosID, value.length);
	
	transaction = RLOG->GetTransaction();
	
	if (!transaction->IsActive())
		transaction->Begin();
	
	WriteRoundID(buf, paxosID);
	table->Set(transaction, buf, value);
	
	// delete old
	if ((int64_t)(paxosID - logCacheSize) >= 0)
	{
		paxosID -= logCacheSize;
		WriteRoundID(buf, paxosID);
		table->Delete(transaction, buf);
	}
	
	if (commit)
		transaction->Commit();
	
	return true;
}

bool LogCache::Get(uint64_t paxosID, ByteString& value_)
{
	ByteArray<128> buf;

	//buf.Writef("@@round:%U", paxosID);
	WriteRoundID(buf, paxosID);
	if (table->Get(NULL, buf, value))
	{
		value_.Set(value);
		return true;
	}
	return false;
}
