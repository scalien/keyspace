#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "ReplicatedKeyspaceDB.h"
#include "KeyspaceService.h"
#include <assert.h>
#include <stdlib.h>
#include "System/Log.h"
#include "System/Common.h"
#include "AsyncListVisitor.h"

ReplicatedKeyspaceDB::ReplicatedKeyspaceDB()
{
	catchingUp = false;
}

bool ReplicatedKeyspaceDB::Init()
{
	Log_Trace();
	
	ReplicatedLog::Get()->SetReplicatedDB(this);
	
	table = database.GetTable("keyspace");
	
	catchupServer.Init(ReplicatedConfig::Get()->GetPort() + CATCHUP_PORT_OFFSET);
	catchupClient.Init(this, table);

	return true;
}

unsigned ReplicatedKeyspaceDB::GetNodeID()
{
	return ReplicatedLog::Get()->GetNodeID();
}

bool ReplicatedKeyspaceDB::IsMasterKnown()
{
	return ReplicatedLog::Get()->GetMaster();
}

int ReplicatedKeyspaceDB::GetMaster()
{
	return ReplicatedLog::Get()->GetMaster();
}

bool ReplicatedKeyspaceDB::IsMaster()
{
	return ReplicatedLog::Get()->IsMaster();
}

bool ReplicatedKeyspaceDB::Add(KeyspaceOp* op)
{
	bool ret;
	Transaction* transaction;

	// don't allow writes for @@ keys
	if (op->IsWrite() && op->key.length > 2 && op->key.buffer[0] == '@' && op->key.buffer[1] == '@')
		return false;

	if (catchingUp)
		return false;
	
	// reads are handled locally, they don't have to be added to the ReplicatedLog
	if (op->IsGet())
	{
		// only handle GETs if I'm the master and it's safe to do so (I have NOPed)
		if (op->type == KeyspaceOp::GET &&
		   (!ReplicatedLog::Get()->IsMaster() || !ReplicatedLog::Get()->IsSafeDB()))
			return false;
		
		if ((transaction = ReplicatedLog::Get()->GetTransaction()) != NULL)
			if (!transaction->IsActive())
				transaction = NULL;
		
		op->value.Allocate(KEYSPACE_VAL_SIZE);
		ret = table->Get(transaction, op->key, op->value);

		op->service->OnComplete(op, ret);
		return true;
	}
	
	if (op->IsList())
	{
		if ((op->type == KeyspaceOp::LIST || op->type == KeyspaceOp::LISTP) &&
		   (!ReplicatedLog::Get()->IsMaster() || !ReplicatedLog::Get()->IsSafeDB()))
			return false;

		AsyncListVisitor *alv = new AsyncListVisitor(op->prefix, op, op->count);
		MultiDatabaseOp* mdbop = new AsyncMultiDatabaseOp();
		mdbop->Visit(table, *alv);
		dbReader.Add(mdbop);
		return true;
	}
	
	// only handle writes if I'm the master
	if (!ReplicatedLog::Get()->IsMaster())
		return false;
	
	ops.Append(op);
	
	return true;
}

bool ReplicatedKeyspaceDB::Submit()
{
	Log_Trace();
	
	// only handle writes if I'm the master
	if (!ReplicatedLog::Get()->IsMaster())
		return false;

	Log_Message("ops.size() = %d", ops.Length());
	
	if (!ReplicatedLog::Get()->IsAppending() && ReplicatedLog::Get()->IsMaster())
		Append();

	Log_Message("ops.size() = %d", ops.Length());
	
	return true;
}

void ReplicatedKeyspaceDB::Execute(Transaction* transaction, bool ownAppend)
{
	bool		ret;
	int64_t		num;
	unsigned	nread;
	KeyspaceOp*	op;
	KeyspaceOp**it;
	
	ret = true;
	if (msg.type == KEYSPACE_SET)
		ret &= table->Set(transaction, msg.key, msg.value);
	else if (msg.type == KEYSPACE_TESTANDSET)
	{
		ret &= table->Get(transaction, msg.key, data);
		if (data == msg.test)
			ret &= table->Set(transaction, msg.key, msg.value);
		else
			ret = false;
	}
	else if (msg.type == KEYSPACE_ADD)
	{
		ret &= table->Get(transaction, msg.key, data); // read number
		
		if (ret)
		{
			num = strntoint64_t(data.buffer, data.length, &nread); // parse number
			if (nread == (unsigned) data.length)
			{
				num = num + msg.num;
				data.length = snprintf(data.buffer, data.size, "%" PRIi64 "", num); // print number
				ret &= table->Set(transaction, msg.key, data); // write number
			}
			else
				ret = false;
		}
	}
	else if (msg.type == KEYSPACE_DELETE)
		ret &= table->Delete(transaction, msg.key);
	else if (msg.type == KEYSPACE_PRUNE)
		ret &= table->Prune(transaction, msg.prefix);
	else
		ASSERT_FAIL();
	
	if (ownAppend)
	{
		it = ops.Head();
		op = *it;
		if (op->type == KeyspaceOp::DIRTY_GET || op->type == KeyspaceOp::GET)
			ASSERT_FAIL();
		if (op->type == KeyspaceOp::ADD && ret)
		{
			// return new value to client
			op->value.Allocate(data.length);
			op->value.Set(data);

		}
		ops.Remove(op);
		op->service->OnComplete(op, ret);
		}
}

void ReplicatedKeyspaceDB::OnAppend(Transaction* transaction, uint64_t paxosID, ByteString value, bool ownAppend)
{
	unsigned numOps, nread;

	Log_Message("length: %d", value.length);

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
		{
			Log_Message("Failed parsing:");
			Log_Message("%.*s", value.length, value.buffer);
			ASSERT_FAIL();
			break;
		}
	}

	Log_Message("paxosID = %" PRIu64 ", numOps = %u", paxosID, numOps);
	
	Log_Message("ops.size() = %d", ops.Length());
	
	if (ReplicatedLog::Get()->IsMaster() && ops.Length() > 0)
		Append();
		
	Log_Message("ops.size() = %d", ops.Length());
}

void ReplicatedKeyspaceDB::Append()
{
	Log_Trace();
	
	if (ops.Length() == 0)
		return;
	
	ByteString	bs;
	KeyspaceOp*	op;
	KeyspaceOp**it;

	pvalue.length = 0;
	bs = pvalue;

	unsigned numAppended = 0;
	
	for (it = ops.Head(); it != NULL; it = ops.Next(it))
	{
		op = *it;
		
		if (op->appended)
			ASSERT_FAIL();
		
		msg.FromKeyspaceOp(op);
		if (msg.Write(bs))
		{
			pvalue.length += bs.length;
			bs.Advance(bs.length);
			op->appended = true;
			numAppended++;
		}
		else
			break;
	}
	
	if (pvalue.length > 0)
	{
		ReplicatedLog::Get()->Append(pvalue);
		Log_Message("appending %d ops (length: %d)", numAppended, pvalue.length);
	}
}

void ReplicatedKeyspaceDB::FailKeyspaceOps()
{
	Log_Trace();

	KeyspaceOp	**it;
	KeyspaceOp	*op;
	for (it = ops.Head(); it != NULL; /* advanded in body */)
	{
		op = *it;
		
		it = ops.Remove(it);
		op->service->OnComplete(op, false);
	}
	
	if (ops.Length() > 0)
		ASSERT_FAIL();
}

void ReplicatedKeyspaceDB::OnMasterLease(unsigned)
{
	Log_Message("ops.size() = %d", ops.Length());

	if (!ReplicatedLog::Get()->IsAppending() && ReplicatedLog::Get()->IsMaster() && ops.Length() > 0)
		Append();

	Log_Message("ops.size() = %d", ops.Length());
}

void ReplicatedKeyspaceDB::OnMasterLeaseExpired()
{
	Log_Trace();

	Log_Message("ops.size() = %d", ops.Length());
	
	if (!ReplicatedLog::Get()->IsMaster())
		FailKeyspaceOps();
		
	Log_Message("ops.size() = %d", ops.Length());
}

void ReplicatedKeyspaceDB::OnDoCatchup(unsigned nodeID)
{
	Log_Trace();

	catchingUp = true;
	ReplicatedLog::Get()->Stop();
	if (ReplicatedLog::Get()->GetTransaction()->IsActive())
		ReplicatedLog::Get()->GetTransaction()->Commit();
	catchupClient.Start(nodeID);
}

void ReplicatedKeyspaceDB::OnCatchupComplete()
{
	Log_Trace();
	
	catchingUp = false;
	ReplicatedLog::Get()->Continue();
}

void ReplicatedKeyspaceDB::OnCatchupFailed()
{
	Log_Trace();

	catchingUp = false;
	ReplicatedLog::Get()->Continue();	
}
