#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "ReplicatedKeyspaceDB.h"
#include "KeyspaceService.h"
#include <assert.h>
#include <stdlib.h>
#include "System/Log.h"
#include "System/Common.h"
#include "AsyncListVisitor.h"
#include "System/Stopwatch.h"

ReplicatedKeyspaceDB::ReplicatedKeyspaceDB()
:	asyncOnAppend(this, &ReplicatedKeyspaceDB::AsyncOnAppend),
	onAppendComplete(this, &ReplicatedKeyspaceDB::OnAppendComplete),
	asyncAppender(1)
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

	asyncAppender.Start();
	asyncAppenderActive = false;
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
		op->status = table->Get(transaction, op->key, op->value);
		op->service->OnComplete(op);
		return true;
	}
	
	if (op->IsList())
	{
		if ((op->type == KeyspaceOp::LIST || op->type == KeyspaceOp::LISTP) &&
		   (!ReplicatedLog::Get()->IsMaster() || !ReplicatedLog::Get()->IsSafeDB()))
			return false;

		AsyncListVisitor *alv = new AsyncListVisitor(op);
		MultiDatabaseOp* mdbop = new AsyncMultiDatabaseOp();
		mdbop->Visit(table, *alv);
		dbReader.Add(mdbop);
		return true;
	}
	
	// only handle writes if I'm the master
	if (!ReplicatedLog::Get()->IsMaster())
		return false;
	
	ops.Append(op);

#ifdef SUBMIT_HACK
	Submit();
#endif
	
	return true;
}

bool ReplicatedKeyspaceDB::Submit()
{
//	Log_Trace();
	
	// only handle writes if I'm the master
	if (!ReplicatedLog::Get()->IsMaster())
		return false;

	if (!ReplicatedLog::Get()->IsAppending() &&
		ReplicatedLog::Get()->IsMaster() &&
		!asyncAppenderActive)
	{
		Log_Trace("ops.size() = %d", ops.Length());	
		Append();
		Log_Trace("ops.size() = %d", ops.Length());
	}
	
	return true;
}

void ReplicatedKeyspaceDB::OnAppend(Transaction* transaction_,
uint64_t /*paxosID*/, ByteString value_, bool ownAppend_)
{
	Log_Trace();
	
	transaction = transaction_;
	valueBuffer.Set(value_);
	ownAppend = ownAppend_;
	
	ReplicatedLog::Get()->StopPaxos();

	assert(asyncAppenderActive == false);
	asyncAppenderActive = true;
	asyncAppender.Execute(&asyncOnAppend);
}

void ReplicatedKeyspaceDB::AsyncOnAppend()
{
	Log_Trace();
	
	unsigned		nread;
	bool			ret;
	KeyspaceOp*		op;
	KeyspaceOp**	it;
	ByteString		value;
	Stopwatch		sw;

	value = valueBuffer;
	Log_Trace("length: %d", value.length);
	
	numOps = 0;
	if (ownAppend)
		it = ops.Head();
	else
		it = NULL;
		
	while (true)
	{
		if (msg.Read(value, nread))
		{
			sw.Start();
			ret = Execute(transaction);
			sw.Stop();
			value.Advance(nread);
			numOps++;
			
			if (ownAppend)
			{
				op = *it;
				if (op->type == KeyspaceOp::DIRTY_GET || op->type == KeyspaceOp::GET)
					ASSERT_FAIL();
				if ((op->type == KeyspaceOp::ADD ||
					 op->type == KeyspaceOp::TEST_AND_SET ||
					 op->type == KeyspaceOp::REMOVE) && ret)
					op->value.Set(data);
				op->status = ret;
				it = ops.Next(it);
			}
			
			if (value.length == 0)
				break;
		}
		else
		{
			Log_Trace("Failed parsing:");
			Log_Trace("%.*s", value.length, value.buffer);
			ASSERT_FAIL();
			break;
		}
	}
	
	Log_Trace("time spent in Execute(): %ld", sw.elapsed);
	Log_Trace("numOps = %u", numOps);
	
	IOProcessor::Complete(&onAppendComplete);
}

bool ReplicatedKeyspaceDB::Execute(Transaction* transaction)
{
	bool		ret;
	int64_t		num;
	unsigned	nread;
	
	ret = true;
	switch (msg.type)
	{
	case KEYSPACE_SET:
		ret &= table->Set(transaction, msg.key, msg.value);
		break;

	case KEYSPACE_TEST_AND_SET:
		ret &= table->Get(transaction, msg.key, data);
		if (data == msg.test)
		{
			ret &= table->Set(transaction, msg.key, msg.value);
			if (ret)
				data.Set(msg.value);
		}
		break;

	case KEYSPACE_ADD:
		ret &= table->Get(transaction, msg.key, data); // read number
		if (!ret)
			break;
		num = strntoint64(data.buffer, data.length, &nread); // parse number
		if (nread == (unsigned) data.length)
		{
			num = num + msg.num;
			data.length = snwritef(data.buffer, data.size, "%I", num); // print number
			ret &= table->Set(transaction, msg.key, data); // write number
		}
		else
			ret = false;
		break;
		
	case KEYSPACE_RENAME:
		ret &= table->Get(transaction, msg.key, data);
		if (!ret) break;
		ret &= table->Set(transaction, msg.newKey, data);
		if (!ret) break;
		ret &= table->Delete(transaction, msg.key);
		break;

	case KEYSPACE_DELETE:
		ret &= table->Delete(transaction, msg.key);
		break;
		
	case KEYSPACE_REMOVE:
		ret &= table->Get(transaction, msg.key, data);
		ret &= table->Delete(transaction, msg.key);
		break;

	case KEYSPACE_PRUNE:
		ret &= table->Prune(transaction, msg.prefix);
		break;

	default:
		ASSERT_FAIL();
	}
	
	return ret;
}

void ReplicatedKeyspaceDB::OnAppendComplete()
{
	Log_Trace();
	
	unsigned		i;
	KeyspaceOp*		op;
	KeyspaceOp**	it;
	
	if (ownAppend)
	{
		for (i = 0; i < numOps; i++)
		{
			it = ops.Head();
			op = *it;
			ops.Remove(op);
			op->service->OnComplete(op);
		}
	}

	asyncAppenderActive = false;
	
	if (!ReplicatedLog::Get()->IsMaster())
		FailKeyspaceOps();

	ReplicatedLog::Get()->ContinuePaxos();
	if (!ReplicatedLog::Get()->IsAppending() && ReplicatedLog::Get()->IsMaster() && ops.Length() > 0)
		Append();
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
		Log_Trace("appending %d ops (length: %d)", numAppended, pvalue.length);
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
		op->service->OnComplete(op);
	}
	
	if (ops.Length() > 0)
		ASSERT_FAIL();
}

void ReplicatedKeyspaceDB::OnMasterLease(unsigned)
{
	Log_Trace("ops.size() = %d", ops.Length());

	if (!ReplicatedLog::Get()->IsAppending() && ReplicatedLog::Get()->IsMaster() && ops.Length() > 0)
		Append();

	Log_Trace("ops.size() = %d", ops.Length());
}

void ReplicatedKeyspaceDB::OnMasterLeaseExpired()
{
	Log_Trace("ops.size() = %d", ops.Length());
	
	if (!ReplicatedLog::Get()->IsMaster() && !asyncAppenderActive)
		FailKeyspaceOps();
		
	Log_Trace("ops.size() = %d", ops.Length());
}

void ReplicatedKeyspaceDB::OnDoCatchup(unsigned nodeID)
{
	Log_Trace();

	catchingUp = true;
	ReplicatedLog::Get()->StopPaxos();
	ReplicatedLog::Get()->StopMasterLease();
	catchupClient.Start(nodeID, ReplicatedLog::Get()->GetPaxosID());
}

void ReplicatedKeyspaceDB::OnCatchupComplete()
{
	Log_Trace();
	
	catchingUp = false;
	ReplicatedLog::Get()->ContinuePaxos();
	ReplicatedLog::Get()->ContinueMasterLease();
}

void ReplicatedKeyspaceDB::OnCatchupFailed()
{
	Log_Trace();

	catchingUp = false;
	ReplicatedLog::Get()->ContinuePaxos();
	ReplicatedLog::Get()->ContinueMasterLease();
}
