#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "KeyspaceDB.h"
#include <assert.h>
#include <stdlib.h>
#include "System/Log.h"
#include "System/Common.h"

void KeyspaceOp_Alloc::Alloc(KeyspaceOp& op)
{
	type = op.type;
	client = op.client;
	key = op.key;
	value = op.value;
	test = op.test;
	
	pkey = (char*) ::Alloc(key.length);
	key.size = key.length;
	memcpy(pkey, key.buffer, key.length);
	key.buffer = pkey;
	
	if (type == SET || type == TEST_AND_SET)
	{
		pvalue = (char*) ::Alloc(value.length);
		value.size = value.length;
		memcpy(pvalue, value.buffer, value.length);
		value.buffer = pvalue;
	}
	
	if (type == TEST_AND_SET)
	{
		ptest = (char*) ::Alloc(test.length);
		test.size = test.length;
		memcpy(ptest, test.buffer, test.length);
		test.buffer = ptest;
	}		
}

void KeyspaceOp_Alloc::Free()
{
	if (pkey != NULL) free(pkey);
	if (pvalue != NULL) free(pvalue);
	if (ptest != NULL) free(ptest);
}

KeyspaceDB::KeyspaceDB()
{
	catchingUp = false;
}

bool KeyspaceDB::Init()
{
	Log_Trace();
	
	ReplicatedLog::Get()->SetReplicatedDB(this);
	
	table = database.GetTable("keyspace");
	
	catchupServer.Init(PaxosConfig::Get()->port + CATCHUP_PORT_OFFSET);
	catchupClient.Init(this, table);
	
	return true;
}

unsigned KeyspaceDB::GetNodeID()
{
	return ReplicatedLog::Get()->GetNodeID();
}

bool KeyspaceDB::Add(KeyspaceOp& op_)
{
	Log_Trace();

	bool ret;
	Transaction* transaction;
	KeyspaceOp_Alloc op;
	
	if (catchingUp)
		return false;
	
	if ((!ReplicatedLog::Get()->IsMaster() || !ReplicatedLog::Get()->IsSafeDB()) &&
		!(op_.type == KeyspaceOp::GET && op_.key.length > 2 &&
		  op_.key.buffer[0] == '@' && op_.key.buffer[1] == '@'))
			return false;
	
	if (op_.type == KeyspaceOp::GET)
	{
		if ((transaction = ReplicatedLog::Get()->GetTransaction()) != NULL)
			if (!transaction->IsActive())
				transaction = NULL;
		ret = table->Get(transaction, op_.key, data);
		if (ret)
			op_.value = data;

		op_.client->OnComplete(&op_, ret);
		return true;
	}
	
	// copy client buffers
	op.Alloc(op_);
	ops.Append(op);
	
	if (!ReplicatedLog::Get()->IsAppending() && ReplicatedLog::Get()->IsMaster())
		Append();
	
	return true;
}

void KeyspaceDB::Execute(Transaction* transaction, bool ownAppend)
{
	bool						ret;
	KeyspaceOp_Alloc*			it;
	
	ret = true;
	if (msg.type == KEYSPACE_GET && ownAppend)
	{
		//ret &= table->Get(transaction, msg.key, data);
		ASSERT_FAIL(); // GETs are not put in the ReplicatedLog
	}
	else if (msg.type == KEYSPACE_SET)
	{
		ret &= table->Set(transaction, msg.key, msg.value);
	}
	else if (msg.type == KEYSPACE_TESTANDSET)
	{
		ret &= table->Get(transaction, msg.key, data);
		if (data == msg.test)
			ret &= table->Set(transaction, msg.key, msg.value);
		else
			ret = false;
	}
	else
		ASSERT_FAIL();
		
	if (ownAppend)
	{
		it = ops.Head();
		if (it->type == KeyspaceOp::GET)
		{
				it->value = data;
				if (it->pvalue != NULL)
					free(it->pvalue);
				it->pvalue = NULL;
		}
		it->client->OnComplete(it, ret);
		it->Free();
		ops.Remove(it);
	}
}

void KeyspaceDB::OnAppend(Transaction* transaction, ulong64 paxosID, ByteString value, bool ownAppend)
{
	unsigned numOps, nread;

	numOps = 0;
	while (true)
	{
		if (msg.Read(value, nread))
		{
			Execute(transaction, ownAppend);
			value.Advance(nread);
			numOps++;
			if (value.length == 0)
				break;
		}
		else
			break;
	}

	Log_Message("paxosID = %llu, numOps = %u", paxosID, numOps);
	
	if (ReplicatedLog::Get()->IsMaster() && ops.Size() > 0)
		Append();
}

void KeyspaceDB::Append()
{
	ByteString bs;
	KeyspaceOp_Alloc* it;

	data.length = 0;
	bs = data;

	for (it = ops.Head(); it != NULL; it = ops.Next(it))
	{
		msg.BuildFrom(it);
		if (msg.Write(bs))
		{
			data.length += bs.length;
			bs.Advance(bs.length);
			
			if (bs.length <= 0)
				break;
		}
		else
			break;
	}
	
	ReplicatedLog::Get()->Append(data);
}

void KeyspaceDB::OnMasterLease(unsigned)
{
	if (!ReplicatedLog::Get()->IsAppending() && ReplicatedLog::Get()->IsMaster() && ops.Size() > 0)
		Append();
}

void KeyspaceDB::OnMasterLeaseExpired()
{
	Log_Trace();
}

void KeyspaceDB::OnDoCatchup(unsigned nodeID)
{
	Log_Trace();

	catchingUp = true;
	ReplicatedLog::Get()->Stop();
	if (ReplicatedLog::Get()->GetTransaction()->IsActive())
		ReplicatedLog::Get()->GetTransaction()->Commit();
	catchupClient.Start(nodeID);
}

void KeyspaceDB::OnCatchupComplete()
{
	Log_Trace();
	
	catchingUp = false;
	ReplicatedLog::Get()->Continue();
}

void KeyspaceDB::OnCatchupFailed()
{
	Log_Trace();

	catchingUp = false;
	ReplicatedLog::Get()->Continue();	
}
