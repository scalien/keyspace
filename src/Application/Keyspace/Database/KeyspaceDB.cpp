#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "KeyspaceDB.h"
#include <assert.h>
#include <stdlib.h>
#include "System/Log.h"
#include "System/Common.h"

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

bool KeyspaceDB::Add(KeyspaceOp& op)
{
	Log_Trace();

	bool ret;
	Transaction* transaction;
	
	if (catchingUp)
		return false;
	
	// don't allow writes for @@ keys
	if ((op.type == KeyspaceOp::SET || op.type == KeyspaceOp::TEST_AND_SET || op.type == KeyspaceOp::DELETE)
		 && op.key.length > 2 && op.key.buffer[0] == '@' && op.key.buffer[1] == '@')
			return false;
	
	// reads are handled locally, they don't have to be added to the ReplicatedLog
	if (op.type == KeyspaceOp::DIRTY_GET || op.type == KeyspaceOp::GET)
	{
		// only handle GETs if I'm the master and it's safe to do so (I have NOPed)
		if (op.type == KeyspaceOp::GET &&
		   (!ReplicatedLog::Get()->IsMaster() || !ReplicatedLog::Get()->IsSafeDB()))
			return false;
		
		if ((transaction = ReplicatedLog::Get()->GetTransaction()) != NULL)
			if (!transaction->IsActive())
				transaction = NULL;
		
		op.value.Allocate(VAL_SIZE);
		ret = table->Get(transaction, op.key, op.value);

		op.client->OnComplete(&op, ret);
		return true;
	}
	
	// only handle writes if I'm the master
	if (!ReplicatedLog::Get()->IsMaster())
		return false;
	
	ops.Append(op);
	
	if (!ReplicatedLog::Get()->IsAppending() && ReplicatedLog::Get()->IsMaster())
		Append();
	
	return true;
}

void KeyspaceDB::Execute(Transaction* transaction, bool ownAppend)
{
	bool		ret;
	KeyspaceOp* it;
	
	ret = true;
	if (msg.type == KEYSPACE_GET)
		ASSERT_FAIL(); // GETs are not put in the ReplicatedLog
	else if (msg.type == KEYSPACE_SET)
		ret &= table->Set(transaction, msg.key, msg.value);
	else if (msg.type == KEYSPACE_TESTANDSET)
	{
		ret &= table->Get(transaction, msg.key, data);
		if (data == msg.test)
			ret &= table->Set(transaction, msg.key, msg.value);
		else
			ret = false;
	}
	else if (msg.type == KEYSPACE_DELETE)
		ret &= table->Delete(transaction, msg.key);
	else
		ASSERT_FAIL();
		
	if (ownAppend)
	{
		it = ops.Head();
		if (it->type == KeyspaceOp::DIRTY_GET || it->type == KeyspaceOp::GET)
			ASSERT_FAIL();
		it->client->OnComplete(it, ret);
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
	ByteString	bs;
	KeyspaceOp* it;

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
