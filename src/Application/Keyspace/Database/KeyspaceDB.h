#ifndef KEYSPACEDB_H
#define KEYSPACEDB_H

#include "KeyspaceConsts.h"
#include "../Protocol/ProtocolServer.h"

#define LISTWORKER_TIMEOUT      1

class KeyspaceOp;

class KeyspaceDB
{
public:
	virtual ~KeyspaceDB() {}
	
	virtual bool		Init() = 0;
	virtual void		Shutdown() = 0;
	virtual bool		Add(KeyspaceOp* op) = 0;
	virtual bool		Submit() = 0;
	virtual unsigned	GetNodeID() = 0;
	virtual bool		IsMasterKnown() = 0;
	virtual int			GetMaster() = 0;
	virtual bool		IsMaster() = 0;
	virtual bool		IsReplicated() = 0;
	virtual void		SetProtocolServer(ProtocolServer* pserver) = 0;
	
	static void WriteValue(
	ByteString &target, uint64_t paxosID, uint64_t commandID, ByteString value)
	{
		if (!target.Writef("%U:%U:%B", paxosID, commandID,
		value.length, value.buffer))
			ASSERT_FAIL();
	}

	static void WriteValue(
	ByteString &target, uint64_t paxosID, uint64_t commandID, uint64_t value)
	{
		if (!target.Writef("%U:%U:%U", paxosID, commandID, value))
			ASSERT_FAIL();
	}

	static void ReadValue(
	ByteString source, uint64_t &paxosID, uint64_t &commandID, ByteString &value)
	{
		unsigned num;
		unsigned len;
		char* p;
		
		p = source.buffer;
		len = source.length;
		paxosID = strntouint64(p, len, &num);
		
		if (num == 0)
			ASSERT_FAIL();
		
		p += num;
		len -= num;
		
		if (*p != ':')
			ASSERT_FAIL();
			
		p++;
		len--;

		commandID = strntouint64(p, len, &num);
		
		if (num == 0)
			ASSERT_FAIL();
		
		p += num;
		len -= num;
		
		if (*p != ':')
			ASSERT_FAIL();
		
		p++;
		len--;
		
		value.size = len;
		value.length = len;
		value.buffer = p;
	}

	static void WriteExpiryTime(
	ByteString &target, uint64_t expiryTime, ByteString key)
	{
		DynArray<32> t;
		// 2^64 is at most 19 digits
		t.length = snprintf(t.buffer, t.size, "%020" PRIu64 "", expiryTime);
	
		if (!target.Writef("!!t:%B:%B", t.length, t.buffer, key.length, key.buffer))
			ASSERT_FAIL();
	}

	static void WriteExpiryKey(
	ByteString &target, ByteString key)
	{
		if (!target.Writef("!!k:%B", key.length, key.buffer))
			ASSERT_FAIL();
	}

	static void ReadExpiryTime(
	ByteString source, uint64_t &expiryTime, ByteString &key)
	{
		unsigned nread;
		
		// skip !!t:
		source.buffer += 4;
		source.length -= 4;
		
		expiryTime = strntouint64(source.buffer, source.length, &nread);
		
		// skip <expiryTime>:
		source.buffer += nread + 1;
		source.length -= nread + 1;
		
		key = source; 
	}

	static void ReadExpiryKey(
	ByteString source, ByteString &key)
	{
		// skip !!k:
		source.buffer += 4;
		source.length -= 4;
		
		key = source; 
	}
};


#endif
