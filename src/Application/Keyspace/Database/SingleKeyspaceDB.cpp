#include "SingleKeyspaceDB.h"
#include "KeyspaceService.h"
#include <assert.h>
#include <stdlib.h>
#include "System/Log.h"
#include "System/Common.h"
#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "AsyncListVisitor.h"

bool SingleKeyspaceDB::Init()
{
	Log_Trace();
	
	table = database.GetTable("keyspace");
	writePaxosID = true;
	
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
	uint64_t		storedPaxosID;
	uint64_t		storedCommandID;
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
		op->status &= table->Get(NULL, op->key, data);
		if (op->status)
		{
			ReadValue(data, storedPaxosID, storedCommandID, userValue);
			op->value.Set(userValue);
		}
		op->service->OnComplete(op);
	}
	else if (op->IsList() || op->IsCount())
	{
		AsyncListVisitor *alv = new AsyncListVisitor(op);
		MultiDatabaseOp* mdbop = new AsyncMultiDatabaseOp();
		mdbop->Visit(table, *alv);
		dbReader.Add(mdbop);
	}
	else if (op->type == KeyspaceOp::SET)
	{
		WriteValue(data, 1, 0, op->value);
		op->status &= table->Set(&transaction, op->key, data);
		ops.Append(op);
	}
	else if (op->type == KeyspaceOp::TEST_AND_SET)
	{
		op->status &= table->Get(&transaction, op->key, data);
		if (op->status)
		{
			ReadValue(data, storedPaxosID, storedCommandID, userValue);
			if (userValue == op->test)
			{
				WriteValue(data, 1, 0, op->value);
				op->status &= table->Set(&transaction, op->key, data);
			}
			else
				op->value.Set(userValue);
		}
		ops.Append(op);
	}
	else if (op->type == KeyspaceOp::ADD)
	{
		// read number:
		op->status = table->Get(&transaction, op->key, data);
		if (op->status)
		{
			ReadValue(data, storedPaxosID, storedCommandID, userValue);
			// parse number:
			num = strntoint64(userValue.buffer, userValue.length, &nread);
			if (nread == (unsigned) userValue.length)
			{
				num = num + op->num;
				 // print number:
				data.length = snwritef(data.buffer, data.size, "1:0:%I", num);
				 // write number:
				op->status &= table->Set(&transaction, op->key, data);
				// returned to the user:
				data.length = snwritef(data.buffer, data.size, "%I", num);
				op->value.Allocate(data.length);
				op->value.Set(data);
			}
			else
				op->status = false;
		}
		ops.Append(op);
	}
	else if (op->type == KeyspaceOp::RENAME)
	{
		op->status &= table->Get(&transaction, op->key, data);
		if (op->status)
		{
			// value doesn't change
			op->status &= table->Set(&transaction, op->newKey, data);
			if (op->status)
				op->status &= table->Delete(&transaction, op->key);
		}
		ops.Append(op);
	}
	else if (op->type == KeyspaceOp::DELETE)
	{
		op->status &= table->Delete(&transaction, op->key);
		ops.Append(op);
	}
	else if (op->type == KeyspaceOp::REMOVE)
	{
		op->value.Allocate(KEYSPACE_VAL_SIZE);
		op->status &= table->Get(&transaction, op->key, data);
		if (op->status)
		{
			ReadValue(data, storedPaxosID, storedCommandID, userValue);
			op->value.Set(userValue);
			op->status &= table->Delete(&transaction, op->key);
		}
		ops.Append(op);
	}

	else if (op->type == KeyspaceOp::PRUNE)
	{
		op->status &= table->Prune(&transaction, op->prefix);
		ops.Append(op);
	}
	else
		ASSERT_FAIL();

	return true;
}

bool SingleKeyspaceDB::Submit()
{
	Log_Trace();

	int i, numOps;
	KeyspaceOp* op;
	KeyspaceOp** it;
	
	if (transaction.IsActive())
		transaction.Commit();

	numOps = ops.Length();
	for (i = 0; i < numOps; i++)
	{
		it = ops.Head();
		op = *it;
		ops.Remove(op);
		op->service->OnComplete(op);
	}
		
	return true;
}
