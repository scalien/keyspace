#include "SingleKeyspaceDB.h"
#include "KeyspaceService.h"
#include <assert.h>
#include <stdlib.h>
#include "System/Log.h"
#include "System/Common.h"
#include "System/Events/EventLoop.h"
//#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "SyncListVisitor.h"

SingleKeyspaceDB::SingleKeyspaceDB() :
	onExpiryTimer(this, &SingleKeyspaceDB::OnExpiryTimer),
	expiryTimer(&onExpiryTimer),
    onListWorkerTimeout(this, &SingleKeyspaceDB::OnListWorkerTimeout),
    listTimer(LISTWORKER_TIMEOUT, &onListWorkerTimeout)
{
}

bool SingleKeyspaceDB::Init()
{
	Log_Trace();
	
	table = database.GetTable("keyspace");
	writePaxosID = true;
	
	InitExpiryTimer();
	
	return true;
}

void SingleKeyspaceDB::Shutdown()
{
}

unsigned SingleKeyspaceDB::GetNodeID()
{
	return 0;
}

bool SingleKeyspaceDB::IsMasterKnown()
{
	return true;
}

int SingleKeyspaceDB::GetMaster()
{
	return 0;
}

bool SingleKeyspaceDB::IsMaster()
{
	return true;
}

bool SingleKeyspaceDB::Add(KeyspaceOp* op)
{
//	Log_Trace();
	
	bool			isWrite;
	int64_t			num;
	unsigned		nread;
	uint64_t		storedPaxosID, storedCommandID, expiryTime;
	ByteString		userValue;
		
	isWrite = op->IsWrite();
	
	if (isWrite && !transaction.IsActive())
	{
		transaction.Set(table);
		transaction.Begin();
	}
	
	op->status = true;
	if (op->IsWrite() && writePaxosID)
	{
		if (table->Set(&transaction, "@@paxosID", "1"))
			writePaxosID = false;
	}
	
	if (op->IsGet())
	{
		op->value.Allocate(KEYSPACE_VAL_SIZE);
		op->status &= table->Get(NULL, op->key, vdata);
		if (op->status)
		{
			ReadValue(vdata, storedPaxosID, storedCommandID, userValue);
			op->value.Set(userValue);
		}
		op->service->OnComplete(op);
	}
	else if (op->IsList() || op->IsCount())
	{
        // always append list-type ops
        listOps.Append(op);
        
        OnListWorkerTimeout();
        return true;    
	}
	else if (op->type == KeyspaceOp::SET)
	{
		WriteValue(vdata, 1, 0, op->value);
		op->status &= table->Set(&transaction, op->key, vdata);
		op->service->OnComplete(op);
	}
	else if (op->type == KeyspaceOp::TEST_AND_SET)
	{
		op->status &= table->Get(&transaction, op->key, vdata);
		if (op->status)
		{
			ReadValue(vdata, storedPaxosID, storedCommandID, userValue);
			if (userValue == op->test)
			{
				WriteValue(vdata, 1, 0, op->value);
				op->status &= table->Set(&transaction, op->key, vdata);
			}
			else
				op->value.Set(userValue);
		}
		op->service->OnComplete(op);
	}
	else if (op->type == KeyspaceOp::ADD)
	{
		// read number:
		op->status = table->Get(&transaction, op->key, vdata);
		if (op->status)
		{
			ReadValue(vdata, storedPaxosID, storedCommandID, userValue);
			// parse number:
			num = strntoint64(userValue.buffer, userValue.length, &nread);
			if (nread == (unsigned) userValue.length)
			{
				num = num + op->num;
				 // print number:
				vdata.length = snwritef(vdata.buffer, vdata.size, "1:0:%I", num);
				 // write number:
				op->status &= table->Set(&transaction, op->key, vdata);
				// returned to the user:
				vdata.length = snwritef(vdata.buffer, vdata.size, "%I", num);
				op->value.Allocate(vdata.length);
				op->value.Set(vdata);
			}
			else
				op->status = false;
		}
		op->service->OnComplete(op);
	}
	else if (op->type == KeyspaceOp::RENAME)
	{
		op->status &= table->Get(&transaction, op->key, vdata);
		if (op->status)
		{
			// value doesn't change
			op->status &= table->Set(&transaction, op->newKey, vdata);
			if (op->status)
				op->status &= table->Delete(&transaction, op->key);
		}
		op->service->OnComplete(op);
	}
	else if (op->type == KeyspaceOp::DELETE)
	{
		op->status &= table->Delete(&transaction, op->key);
		op->service->OnComplete(op);
	}
	else if (op->type == KeyspaceOp::REMOVE)
	{
		op->value.Allocate(KEYSPACE_VAL_SIZE);
		op->status &= table->Get(&transaction, op->key, vdata);
		if (op->status)
		{
			ReadValue(vdata, storedPaxosID, storedCommandID, userValue);
			op->value.Set(userValue);
			op->status &= table->Delete(&transaction, op->key);
		}
		op->service->OnComplete(op);
	}

	else if (op->type == KeyspaceOp::PRUNE)
	{
		op->status &= table->Prune(&transaction, op->prefix);
		op->service->OnComplete(op);
	}
	else if (op->type == KeyspaceOp::SET_EXPIRY)
	{
		Log_Trace("Setting expiry for key: %.*s", op->key.length, op->key.buffer);
		// check old expiry
		WriteExpiryKey(kdata, op->key);
		if (table->Get(&transaction, kdata, vdata))
		{
			// this key already had an expiry
			expiryTime = strntouint64(vdata.buffer, vdata.length, &nread);
			if (nread < 1)
				ASSERT_FAIL();
			// delete old value
			WriteExpiryTime(kdata, expiryTime, op->key);
			table->Delete(&transaction, kdata);
		}
		// write !!t:<expirytime>:<key> => NULL
		WriteExpiryTime(kdata, op->nextExpiryTime, op->key);
		op->value.Clear();
		table->Set(&transaction, kdata, op->value);
		// write !!k:<key> => <expiryTime>
		WriteExpiryKey(kdata, op->key);
		table->Set(&transaction, kdata, op->nextExpiryTime);
		InitExpiryTimer();
		op->status = true;
		op->service->OnComplete(op);
	}
	else if (op->type == KeyspaceOp::REMOVE_EXPIRY)
	{
		Log_Trace("Removing expiry for key: %.*s", op->key.length, op->key.buffer);
		// check old expiry
		WriteExpiryKey(kdata, op->key);
		if (table->Get(&transaction, kdata, vdata))
		{
			// this key already had an expiry
			expiryTime = strntouint64(vdata.buffer, vdata.length, &nread);
			if (nread < 1)
				ASSERT_FAIL();
			// delete old value
			WriteExpiryTime(kdata, expiryTime, op->key);
			table->Delete(&transaction, kdata);
		}
		WriteExpiryKey(kdata, op->key);
		table->Delete(&transaction, kdata);
		InitExpiryTimer();
		op->status = true;
		op->service->OnComplete(op);
	}
	else if (op->type == KeyspaceOp::CLEAR_EXPIRIES)
	{
		Log_Trace("Clearing all expiries");
		kdata.Writef("!!");
		table->Prune(&transaction, kdata, true);
		InitExpiryTimer();
		op->status = true;
		op->service->OnComplete(op);		
	}
	else
		ASSERT_FAIL();

	return true;
}

bool SingleKeyspaceDB::Submit()
{
	Log_Trace();

	if (transaction.IsActive())
		transaction.Commit();
		
	return true;
}

void SingleKeyspaceDB::InitExpiryTimer()
{
	uint64_t	expiryTime;
	Cursor		cursor;
	ByteString	key;

	Log_Trace();
	EventLoop::Remove(&expiryTimer);	
	
	table->Iterate(NULL, cursor);
	
	kdata.Set("!!t:");
	if (!cursor.Start(kdata))
		return;
	cursor.Close();
	
	if (kdata.length < 2)
		return;
	
	if (kdata.buffer[0] != '!' || kdata.buffer[1] != '!')
		return;

	ReadExpiryTime(kdata, expiryTime, key);

	expiryTimer.Set(expiryTime);
	EventLoop::Add(&expiryTimer);
}

void SingleKeyspaceDB::OnExpiryTimer()
{
	uint64_t	expiryTime;
	Cursor		cursor;
	ByteString	key;

	Log_Trace();
	
	table->Iterate(NULL, cursor);	
	kdata.Set("!!t:");
	if (!cursor.Start(kdata))
		ASSERT_FAIL();
	cursor.Close();
	
	if (kdata.length < 2)
		ASSERT_FAIL();
	
	if (kdata.buffer[0] != '!' || kdata.buffer[1] != '!')
		ASSERT_FAIL();

	ReadExpiryTime(kdata, expiryTime, key);
	table->Delete(NULL, kdata);
	table->Delete(NULL, key);

	WriteExpiryKey(kdata, key);
	table->Delete(NULL, kdata);
	
	Log_Trace("Expiring key: %.*s", key.length, key.buffer);

	InitExpiryTimer();
}



void SingleKeyspaceDB::ExecuteListWorkers()
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

void SingleKeyspaceDB::ExecuteListWorker(KeyspaceOp** it)
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

void SingleKeyspaceDB::OnListWorkerTimeout()
{
    Log_Trace();
    
    ExecuteListWorkers();
    
    if (listOps.length > 0)
        EventLoop::Reset(&listTimer);
}
