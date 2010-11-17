//#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "ReplicatedKeyspaceDB.h"
#include "KeyspaceService.h"
#include <assert.h>
#include <stdlib.h>
#include "System/Log.h"
#include "System/Common.h"
//#include "AsyncListVisitor.h"
#include "SyncListVisitor.h"
#include "System/Stopwatch.h"

ReplicatedKeyspaceDB::ReplicatedKeyspaceDB()
:	asyncOnAppend(this, &ReplicatedKeyspaceDB::AsyncOnAppend),
	onAppendComplete(this, &ReplicatedKeyspaceDB::OnAppendComplete),
	onExpiryTimer(this, &ReplicatedKeyspaceDB::OnExpiryTimer),
	expiryTimer(&onExpiryTimer),
    onListWorkerTimeout(this, &ReplicatedKeyspaceDB::OnListWorkerTimeout),
    listTimer(LISTWORKER_TIMEOUT, &onListWorkerTimeout)
{
	asyncAppender = ThreadPool::Create(1);
	catchingUp = false;
	transaction = NULL;
	expiryAdded = false;
}

ReplicatedKeyspaceDB::~ReplicatedKeyspaceDB()
{
	delete asyncAppender;
}

bool ReplicatedKeyspaceDB::Init()
{
	Log_Trace();
	
	RLOG->SetReplicatedDB(this);
	
	table = database.GetTable("keyspace");
	
	catchupServer.Init(RCONF->GetPort() + CATCHUP_PORT_OFFSET);
	catchupClient.Init(this, table);

	estimatedLength = 0;

	asyncAppender->Start();
	asyncAppenderActive = false;
	
	deleteDB = false;

	return true;
}

void ReplicatedKeyspaceDB::Shutdown()
{
	asyncAppender->Stop();
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
	// don't allow writes for !! keys
	if (op->IsWrite() && op->key.length > 2 &&
		op->key.buffer[0] == '!' && op->key.buffer[1] == '!')
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

        // avoid concurrent read/writes
        if (asyncAppenderActive || RLOG->IsWriting())
        {
            getOps.Append(op);
            return true;
        }

		op->value.Allocate(KEYSPACE_VAL_SIZE);
		op->status = table->Get(NULL, op->key, rdata);
		if (op->status)
		{
			ReadValue(rdata, storedPaxosID, storedCommandID, userValue);
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

        // always append list-type ops
        listOps.Append(op);

        // avoid concurrent read/writes
        if (asyncAppenderActive || RLOG->IsWriting())
            return true;
            
        OnListWorkerTimeout();
        return true;
	}
	
	// only handle writes if I'm the master
	if (!RLOG->IsMaster())
		return false;
	
	if (op->IsExpiry())
		expiryAdded = true;
	
	writeOps.Append(op);

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

	if (!RLOG->IsAppending() &&
		RLOG->IsMaster() &&
		!asyncAppenderActive)
	{
		Log_Trace("writeOps.size() = %d", writeOps.Length());	
		Append();
		Log_Trace("writeOps.size() = %d", writeOps.Length());
	}
	
	return true;
}

void ReplicatedKeyspaceDB::OnAppend(Transaction* transaction_,
uint64_t paxosID_, ByteString value_, bool ownAppend_)
{
	Log_Trace();
	
	paxosID = paxosID_;
	transaction = transaction_;
	valueBuffer.Set(value_);
	ownAppend = ownAppend_;
	
	RLOG->StopPaxos();

	assert(asyncAppenderActive == false);
	asyncAppenderActive = true;
	asyncAppender->Execute(&asyncOnAppend);
}


void ReplicatedKeyspaceDB::AsyncOnAppend()
{
	Log_Trace();
	
	unsigned		nread;
	bool			ret;
	uint64_t		commandID;
	KeyspaceOp*		op;
	KeyspaceOp**	it;
	ByteString		value;
	Stopwatch		sw;

	value.Set(valueBuffer);
	Log_Trace("length: %d", value.length);
	
	numOps = 0;
	if (ownAppend)
		it = writeOps.Head();
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
						op->value.Set(wdata);
				op->status = ret;
				it = writeOps.Next(it);
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
	Log_Trace("ops = %u", numOps);
	Log_Trace("ops/sec = %f", (double)1000*numOps/sw.elapsed);
	
	IOProcessor::Complete(&onAppendComplete);
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
	uint64_t	storedPaxosID, storedCommandID;
	ByteString	userValue;
	ValBuffer	tmp;
	ByteString	key;
	
	ret = true;
	switch (msg.type)
	{
	case KEYSPACE_SET:
		WriteValue(wdata, paxosID, commandID, msg.value);
		ret &= table->Set(transaction, msg.key, wdata);
		wdata.Set(msg.value);
		break;

	case KEYSPACE_TEST_AND_SET:
		ret &= table->Get(transaction, msg.key, wdata);
		if (!ret) break;
		ReadValue(wdata, storedPaxosID, storedCommandID, userValue);
		CHECK_CMD();
		wdata.Set(userValue);
		if (wdata == msg.test)
		{
			WriteValue(wdata, paxosID, commandID, msg.value);
			ret &= table->Set(transaction, msg.key, wdata);
			if (ret)
				wdata.Set(msg.value);
		}
		break;

	case KEYSPACE_ADD:
		// read number:
		ret &= table->Get(transaction, msg.key, wdata);
		if (!ret) break;
		ReadValue(wdata, storedPaxosID, storedCommandID, userValue);
		CHECK_CMD();
		// parse number:
		num = strntoint64(userValue.buffer, userValue.length, &nread);
		if (nread == (unsigned) userValue.length)
		{
			num = num + msg.num;
			// print number:
			wdata.length = snwritef(wdata.buffer, wdata.size, "%U:%U:%I",
								   paxosID, commandID, num);
			// write number:
			ret &= table->Set(transaction, msg.key, wdata);
			// data is returned to the user
			wdata.length = snwritef(wdata.buffer, wdata.size, "%I", num);
		}
		else
			ret = false;
		break;
		
	case KEYSPACE_RENAME:
		ret &= table->Get(transaction, msg.key, wdata);
		if (!ret) break;
		ReadValue(wdata, storedPaxosID, storedCommandID, userValue);
		CHECK_CMD();
		tmp.Set(userValue);
		WriteValue(wdata, paxosID, commandID, tmp);
		ret &= table->Set(transaction, msg.newKey, wdata);
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
		wdata.Set(userValue);
		ret &= table->Delete(transaction, msg.key);
		break;

	case KEYSPACE_PRUNE:
		ret &= table->Prune(transaction, msg.prefix);
		break;

	case KEYSPACE_SET_EXPIRY:
		Log_Trace("Setting expiry for key: %.*s", msg.key.length, msg.key.buffer);
		// remove old expiry
		if (msg.prevExpiryTime > 0)
		{
			WriteExpiryTime(kdata, msg.prevExpiryTime, msg.key);
			table->Delete(transaction, kdata);			
		}
		// write !!t:<expirytime>:<key> => NULL
		WriteExpiryTime(kdata, msg.nextExpiryTime, msg.key);
		rdata.Clear();
		table->Set(transaction, kdata, rdata);
		// write !!k:<key> => <expiryTime>
		WriteExpiryKey(kdata, msg.key);
		table->Set(transaction, kdata, msg.nextExpiryTime);
		ret = true;
		break;

	case KEYSPACE_EXPIRE:
		Log_Trace("Expiring key: %.*s", msg.key.length, msg.key.buffer);
		// delete !!k:<key> => <expiryTime>
		WriteExpiryKey(kdata, msg.key);
		table->Delete(transaction, kdata);
		// delete !!t:<expirytime>:<key> => NULL
		WriteExpiryTime(kdata, msg.prevExpiryTime, msg.key);
		table->Delete(transaction, kdata);
		// delete actual key
		table->Delete(transaction, msg.key); // (*)
		expiryAdded = false;
		ret = true;
		break;

	case KEYSPACE_REMOVE_EXPIRY:
		// same as above except (*) is missing
		Log_Trace("Removing expiry for key: %.*s", msg.key.length, msg.key.buffer);
		// delete !!k:<key> => <expiryTime>
		WriteExpiryKey(kdata, msg.key);
		table->Delete(transaction, kdata);
		// delete !!t:<expirytime>:<key> => NULL
		WriteExpiryTime(kdata, msg.prevExpiryTime, msg.key);
		table->Delete(transaction, kdata);
		ret = true;
		break;

	case KEYSPACE_CLEAR_EXPIRIES:
		Log_Trace("Clearing all expiries");
		kdata.Writef("!!");
		table->Prune(transaction, kdata, true);
		ret = true;
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
		Log_Trace("my append");
		for (i = 0; i < numOps; i++)
		{
			it = writeOps.Head();
			op = *it;
			writeOps.Remove(op);
			if (op->service)
				op->service->OnComplete(op);
			else
			{
				assert(op->type == KeyspaceOp::EXPIRE);
				delete op;
			}
		}
	}
	else
		Log_Trace("not my append");

	asyncAppenderActive = false;
	if (RLOG->IsSafeDB())
		InitExpiryTimer();

	if (!RLOG->IsMaster())
		FailKeyspaceOps();
    else
        ExecuteReadOps();

	RLOG->ContinuePaxos();
	if (!RLOG->IsAppending() && RLOG->IsMaster() && writeOps.Length() > 0)
		Append();
}

void ReplicatedKeyspaceDB::Append()
{
	ByteString	bs;
	KeyspaceOp*	op;
	KeyspaceOp**it;
	uint64_t expiryTime;

	Log_Trace();
	
	if (writeOps.Length() == 0)
		return;
	
	pvalue.length = 0;
	bs.Set(pvalue);

	unsigned numAppended = 0;
	
	for (it = writeOps.Head(); it != NULL; it = writeOps.Next(it))
	{
		op = *it;
		
		if (op->appended)
			ASSERT_FAIL();
		
		if (op->IsExpiry() && op->type != KeyspaceOp::CLEAR_EXPIRIES)
		{
			// at this point we have up-to-date info on the expiry time
			expiryTime = GetExpiryTime(op->key);
			op->prevExpiryTime = expiryTime;
		}

		
		msg.FromKeyspaceOp(op);
		if (msg.Write(bs))
		{
			pvalue.length += bs.length;
			bs.Advance(bs.length);
			op->appended = true;
			numAppended++;
			if (op->IsExpiry())
			{
				// one expiry command per paxos round
				break;
			}
		}
		else
			break;
	}
	
	if (pvalue.length > 0)
	{
		estimatedLength -= pvalue.length;
		if (estimatedLength < 0) estimatedLength = 0;
		RLOG->Append(pvalue);
		Log_Trace("appending %d writeOps (length: %d)", numAppended, pvalue.length);
	}
}

void ReplicatedKeyspaceDB::FailKeyspaceOps()
{
    FailReadOps();
    FailWriteOps();
}

void ReplicatedKeyspaceDB::FailReadOps()
{
	Log_Trace();

	KeyspaceOp	**it;
	KeyspaceOp	*op;

	for (it = getOps.Head(); it != NULL; /* advanded in body */)
	{
		op = *it;
		
		it = getOps.Remove(it);
		op->status = false;
		if (op->service)
			op->service->OnComplete(op);
    }
    
    // TODO: fail LIST ops
}

void ReplicatedKeyspaceDB::FailWriteOps()
{
	Log_Trace();

	KeyspaceOp	**it;
	KeyspaceOp	*op;

	for (it = writeOps.Head(); it != NULL; /* advanded in body */)
	{
		op = *it;
		
		it = writeOps.Remove(it);
		op->status = false;
		if (op->service)
			op->service->OnComplete(op);
		else
		{
			assert(op->type == KeyspaceOp::EXPIRE);
			delete op;
		}
	}

	expiryAdded = false;

	if (writeOps.Length() > 0)
		ASSERT_FAIL();
}

void ReplicatedKeyspaceDB::OnMasterLease()
{
	InitExpiryTimer();
}

void ReplicatedKeyspaceDB::OnMasterLeaseExpired()
{
	Log_Trace("writeOps.size() = %d", writeOps.Length());
	
	if (!RLOG->IsMaster() && !asyncAppenderActive)
		FailKeyspaceOps();
		
	Log_Trace("writeOps.size() = %d", writeOps.Length());
	
	EventLoop::Remove(&expiryTimer);
}

void ReplicatedKeyspaceDB::OnDoCatchup(unsigned nodeID)
{
	Log_Trace();

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

	Log_Message("Catchup started from node %d", nodeID);

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

	Log_Message("Truncating database");
	deleteDB = true;
	EventLoop::Stop();
}

void ReplicatedKeyspaceDB::SetProtocolServer(ProtocolServer* pserver)
{
	pservers.Append(pserver);
}

bool ReplicatedKeyspaceDB::DeleteDB()
{
	return deleteDB;
}

void ReplicatedKeyspaceDB::OnExpiryTimer()
{
	assert(RLOG->IsMaster());
	
	uint64_t	expiryTime;
	Cursor		cursor;
	ByteString	key;
	KeyspaceOp*	op;

	Log_Trace();
	
	if (expiryAdded)
		return;
	
	transaction = RLOG->GetTransaction();
	if (!transaction->IsActive())
		transaction->Begin();
	
	table->Iterate(transaction, cursor);	
	kdata.Set("!!t:");
	if (!cursor.Start(kdata))
		ASSERT_FAIL();
	cursor.Close();
	
	if (kdata.length < 2)
		ASSERT_FAIL();
	
	if (kdata.buffer[0] != '!' || kdata.buffer[1] != '!')
		ASSERT_FAIL();

	ReadExpiryTime(kdata, expiryTime, key);
	
	op = new KeyspaceOp;
	op->cmdID = 0;
	op->type = KeyspaceOp::EXPIRE;
	op->key.Allocate(key.length);
	op->key.Set(key);
	// expiryTime is set in Append()
	op->service = NULL;
	Add(op);
	Submit();
}

void ReplicatedKeyspaceDB::InitExpiryTimer()
{
	uint64_t	expiryTime;
	Cursor		cursor;
	ByteString	key;

	Log_Trace();
	
	EventLoop::Remove(&expiryTimer);
	
	transaction = RLOG->GetTransaction();
	if (!transaction->IsActive())
		transaction->Begin();

	table->Iterate(transaction, cursor);
	
	kdata.Set("!!t:");
	if (!cursor.Start(kdata))
	{
		cursor.Close();
		return;
	}
	cursor.Close();
	
	if (kdata.length < 2)
		return;
	
	if (kdata.buffer[0] != '!' || kdata.buffer[1] != '!')
		return;

	ReadExpiryTime(kdata, expiryTime, key);
	Log_Trace("Setting expiry for %.*s at %" PRIu64 "", key.length, key.buffer, expiryTime);

	expiryTimer.Set(expiryTime);
	EventLoop::Add(&expiryTimer);
}

uint64_t ReplicatedKeyspaceDB::GetExpiryTime(ByteString key)
{
	uint64_t expiryTime;
	ByteString userValue;
	unsigned nread;
	
	WriteExpiryKey(kdata, key);
	if (table->Get(RLOG->GetTransaction(), kdata, rdata))
	{
		Log_Trace("read %.*s => %.*s", kdata.length, kdata.buffer, rdata.length, rdata.buffer);
		// the key has an expiry
		expiryTime = strntouint64(rdata.buffer, rdata.length, &nread);
		if (nread < 1)
			ASSERT_FAIL();
		return expiryTime;
	}
	else
		return 0;
}

void ReplicatedKeyspaceDB::ExecuteReadOps()
{
    Log_Trace();
    
    ExecuteGetOps();
    ExecuteListWorkers();
}

void ReplicatedKeyspaceDB::ExecuteGetOps()
{
    Log_Trace();
    
	uint64_t        storedPaxosID, storedCommandID;
	ByteString      userValue;
    KeyspaceOp**    it;
    KeyspaceOp*     op;
    
	for (it = getOps.Head(); it != NULL; /* advanced in body */)
	{
        op = *it;
        
        assert(op->IsGet());
        // only handle GETs if I'm the master and
		// it's safe to do so (I have NOPed)
		if (!RLOG->IsMaster() || !RLOG->IsSafeDB())
        {
            // we lost mastership in the meantime
            op->status = false;
        }
        else
        {
            op->value.Allocate(KEYSPACE_VAL_SIZE);
            op->status = table->Get(NULL, op->key, rdata);
            if (op->status)
            {
                ReadValue(rdata, storedPaxosID, storedCommandID, userValue);
                op->value.Set(userValue);
            }
        }

        it = getOps.Remove(it);
		op->service->OnComplete(op);
    }
}

void ReplicatedKeyspaceDB::ExecuteListWorkers()
{
    Log_Trace();
    
    KeyspaceOp**    it;
    KeyspaceOp**    next;

	for (it = listOps.Head(); it != NULL; it = next)
	{
        next = listOps.Next(it);

        ExecuteListWorker(it); // may delete from list
    }
}

void ReplicatedKeyspaceDB::ExecuteListWorker(KeyspaceOp** it)
{
    Log_Trace();
    
    KeyspaceOp*         op;
    SyncListVisitor     lv(*it);
    
    op = *it;
    
    table->Visit(lv);
    
    op->num += lv.num;
    
    if (!lv.completed)
    {
        op->offset = 1;
        op->count -= lv.num;
        if (op->count != 0)
            return;
    }
    
    listOps.Remove(it);
    if (op->IsCount())
        op->value.Writef("%I", op->num);
    op->status = true;
    op->key.length = 0;
    op->service->OnComplete(op, true);
}

void ReplicatedKeyspaceDB::OnListWorkerTimeout()
{
    Log_Trace();
    
    if (!asyncAppenderActive && !RLOG->IsWriting())
        ExecuteReadOps();
    
    if (getOps.length > 0 || listOps.length > 0)
        EventLoop::Reset(&listTimer);
}
