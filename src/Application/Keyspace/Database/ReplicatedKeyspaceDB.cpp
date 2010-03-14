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
{
	catchingUp = false;
}

ReplicatedKeyspaceDB::~ReplicatedKeyspaceDB()
{
}

bool ReplicatedKeyspaceDB::Init()
{
	Log_Trace();
	
	RLOG->SetReplicatedDB(this);
	
	table = database.GetTable("keyspace");
	
	catchupServer.Init(RCONF->GetPort() + CATCHUP_PORT_OFFSET);
	catchupClient.Init(this, table);

	estimatedLength = 0;
	
	deleteDB = false;

	return true;
}

void ReplicatedKeyspaceDB::Shutdown()
{
	catchupServer.Shutdown();
	catchupClient.Shutdown();
}

unsigned ReplicatedKeyspaceDB::GetNodeID()
{
	return RLOG->GetNodeID();
}

bool ReplicatedKeyspaceDB::IsMasterKnown()
{
	return (RLOG->GetMaster() != -1);
}

int ReplicatedKeyspaceDB::GetMaster()
{
	return RLOG->GetMaster();
}

bool ReplicatedKeyspaceDB::IsMaster()
{
	return RLOG->IsMaster();
}

bool ReplicatedKeyspaceDB::IsCatchingUp()
{
	return catchingUp;
}

bool ReplicatedKeyspaceDB::Add(KeyspaceOp* op)
{
	uint64_t storedPaxosID, storedCommandID;
	ByteString userValue;
	
	// don't allow writes for @@ keys
	if (op->IsWrite() && op->key.length > 2 &&
		op->key.buffer[0] == '@' && op->key.buffer[1] == '@')
			return false;

	if (catchingUp)
		return false;
	
	// reads are handled locally, they don't have to
	// be added to the ReplicatedLog
	if (op->IsGet())
	{
		// only handle GETs if I'm the master and
		// it's safe to do so (I have NOPed)
		if (op->type == KeyspaceOp::GET &&
		   (!RLOG->IsMaster() || !RLOG->IsSafeDB()))
			return false;
				
		op->value.Allocate(KEYSPACE_VAL_SIZE);
		op->status = table->Get(NULL, op->key, data);
		if (op->status)
		{
			ReadValue(data, storedPaxosID, storedCommandID, userValue);
			op->value.Set(userValue);
		}
		op->service->OnComplete(op);
		return true;
	}
	
	if (op->IsList() || op->IsCount())
	{
		if ((op->type == KeyspaceOp::LIST ||
		op->type == KeyspaceOp::LISTP ||
		op->type == KeyspaceOp::COUNT) &&
		(!RLOG->IsMaster() || !RLOG->IsSafeDB()))
			return false;

		AsyncListVisitor *alv = new AsyncListVisitor(op);
		MultiDatabaseOp* mdbop = new AsyncMultiDatabaseOp();
		mdbop->Visit(table, *alv);
		dbReader.Add(mdbop);
		return true;
	}
	
	// only handle writes if I'm the master
	if (!RLOG->IsMaster())
		return false;
	
	ops.Append(op);

	// TODO: huge hack
	if (estimatedLength < PAXOS_SIZE)
	{
		tmp.FromKeyspaceOp(op);
		if (tmp.Write(tmpBuffer))
			estimatedLength += tmpBuffer.length;
	}
	
	if (estimatedLength >= PAXOS_SIZE)
		Submit();
	
	return true;
}

bool ReplicatedKeyspaceDB::Submit()
{
//	Log_Trace();
	
	// only handle writes if I'm the master
	if (!RLOG->IsMaster())
		return false;

	if (!RLOG->IsAppending() && RLOG->IsMaster())
	{
		Log_Trace("ops.size() = %d", ops.Length());	
		Append();
		Log_Trace("ops.size() = %d", ops.Length());
	}
	
	return true;
}

void ReplicatedKeyspaceDB::OnAppend(Transaction* transaction,
uint64_t paxosID, ByteString value_, bool ownAppend)
{
	Log_Trace();
	
	unsigned		i, nread;
	bool			ret;
	uint64_t		commandID;
	KeyspaceOp*		op;
	KeyspaceOp**	it;
	ByteString		value;
	Stopwatch		sw;

	value.Set(value_);
	Log_Trace("length: %d", value.length);
	
	numOps = 0;
	if (ownAppend)
		it = ops.Head();
	else
		it = NULL;
	
	commandID = 0;	
	while (true)
	{
		if (msg.Read(value, nread))
		{
			sw.Start();
			ret = Execute(transaction, paxosID, commandID);
			commandID++;
			sw.Stop();
			value.Advance(nread);
			numOps++;
			
			if (ownAppend)
			{
				op = *it;
				if (op->type == KeyspaceOp::DIRTY_GET ||
					op->type == KeyspaceOp::GET)
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
	Log_Trace("ops/sec = %f", (double)1000*numOps/sw.elapsed);
	
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
	else
		Log_Trace("not my append");

	if (!RLOG->IsMaster())
		FailKeyspaceOps();

	if (!RLOG->IsAppending() && RLOG->IsMaster() && ops.Length() > 0)
		Append();
}

bool ReplicatedKeyspaceDB::Execute(
Transaction* transaction, uint64_t paxosID, uint64_t commandID)
{
#define CHECK_CMD()												\
	if (storedPaxosID > paxosID ||								\
	(storedPaxosID == paxosID && storedCommandID >= commandID))	\
		return true;


	bool		ret;
	unsigned	nread;
	int64_t		num;
	uint64_t	storedPaxosID;
	uint64_t	storedCommandID;
	ByteString	userValue;
	ValBuffer	tmp;
	
	ret = true;
	switch (msg.type)
	{
	case KEYSPACE_SET:
		WriteValue(data, paxosID, commandID, msg.value);
		ret &= table->Set(transaction, msg.key, data);
		data.Set(msg.value);
		break;

	case KEYSPACE_TEST_AND_SET:
		ret &= table->Get(transaction, msg.key, data);
		if (!ret) break;
		ReadValue(data, storedPaxosID, storedCommandID, userValue);
		CHECK_CMD();
		data.Set(userValue);
		if (data == msg.test)
		{
			WriteValue(data, paxosID, commandID, msg.value);
			ret &= table->Set(transaction, msg.key, data);
			if (ret)
				data.Set(msg.value);
		}
		break;

	case KEYSPACE_ADD:
		// read number:
		ret &= table->Get(transaction, msg.key, data);
		if (!ret) break;
		ReadValue(data, storedPaxosID, storedCommandID, userValue);
		CHECK_CMD();
		// parse number:
		num = strntoint64(userValue.buffer, userValue.length, &nread);
		if (nread == (unsigned) userValue.length)
		{
			num = num + msg.num;
			// print number:
			data.length = snwritef(data.buffer, data.size, "%U:%U:%I",
								   paxosID, commandID, num);
			// write number:
			ret &= table->Set(transaction, msg.key, data);
			// data is returned to the user
			data.length = snwritef(data.buffer, data.size, "%I", num);
		}
		else
			ret = false;
		break;
		
	case KEYSPACE_RENAME:
		ret &= table->Get(transaction, msg.key, data);
		if (!ret) break;
		ReadValue(data, storedPaxosID, storedCommandID, userValue);
		CHECK_CMD();
		tmp.Set(userValue);
		WriteValue(data, paxosID, commandID, tmp);
		ret &= table->Set(transaction, msg.newKey, data);
		if (!ret) break;
		ret &= table->Delete(transaction, msg.key);
		break;

	case KEYSPACE_DELETE:
		ret &= table->Delete(transaction, msg.key);
		break;
		
	case KEYSPACE_REMOVE:
		ret &= table->Get(transaction, msg.key, tmp);
		if (!ret) break;
		ReadValue(tmp, storedPaxosID, storedCommandID, userValue);
		CHECK_CMD();
		data.Set(userValue);
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

void ReplicatedKeyspaceDB::Append()
{
	Log_Trace();
	
	if (ops.Length() == 0)
		return;
	
	ByteString	bs;
	KeyspaceOp*	op;
	KeyspaceOp**it;

	pvalue.length = 0;
	bs.Set(pvalue);

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
		estimatedLength -= pvalue.length;
		if (estimatedLength < 0) estimatedLength = 0;
		RLOG->Append(pvalue);
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
		op->status = false;
		op->service->OnComplete(op);
	}
	
	if (ops.Length() > 0)
		ASSERT_FAIL();
}

void ReplicatedKeyspaceDB::OnMasterLease(unsigned)
{
	Log_Trace("ops.size() = %d", ops.Length());

	if (!RLOG->IsAppending() && RLOG->IsMaster() && ops.Length() > 0)
		Append();

	Log_Trace("ops.size() = %d", ops.Length());
}

void ReplicatedKeyspaceDB::OnMasterLeaseExpired()
{
	Log_Trace("ops.size() = %d", ops.Length());
	
	if (!RLOG->IsMaster())
		FailKeyspaceOps();
		
	Log_Trace("ops.size() = %d", ops.Length());
}

void ReplicatedKeyspaceDB::OnDoCatchup(unsigned nodeID)
{
	Log_Trace();

	Log_Message("Catchup started from node %d", nodeID);

	// this is a workaround because BDB truncate is way too slow for any
	// database bigger than 1Gb, as confirmed by BDB workers on forums
//	if (RLOG->GetPaxosID() != 0)
//		RESTART("exiting to truncate database");
	if (RLOG->GetPaxosID() > 0)
	{
		Log_Message("Truncating database");
		deleteDB = true;
		EventLoop::Stop();
		return;
	}
	
	catchingUp = true;
	RLOG->StopPaxos();
	RLOG->StopMasterLease();
	catchupClient.Start(nodeID);
}

void ReplicatedKeyspaceDB::OnCatchupComplete()
{
	Log_Trace();

	Log_Message("Catchup complete");

	catchingUp = false;
	RLOG->ContinuePaxos();
	RLOG->ContinueMasterLease();
}

void ReplicatedKeyspaceDB::OnCatchupFailed()
{
	Log_Trace();

	Log_Message("Catchup failed");

	catchingUp = false;
	RLOG->ContinuePaxos();
	RLOG->ContinueMasterLease();
}

void ReplicatedKeyspaceDB::SetProtocolServer(ProtocolServer* pserver)
{
	pservers.Append(pserver);
}

bool ReplicatedKeyspaceDB::DeleteDB()
{
	return deleteDB;
}
