#ifndef KEYSPACEDB_H
#define KEYSPACEDB_H

#include "KeyspaceConsts.h"
#include "../Protocol/ProtocolServer.h"

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
};


#endif
