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
		op->status &= table->Get(NULL, op->key, op->value);
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
		op->status &= table->Set(&transaction, op->key, op->value);
		ops.Append(op);
	}
	else if (op->type == KeyspaceOp::TEST_AND_SET)
	{
		op->status &= table->Get(&transaction, op->key, data);
		if (data == op->test)
			op->status &= table->Set(&transaction, op->key, op->value);
		else
			op->value.Set(data);		
		ops.Append(op);
	}
	else if (op->type == KeyspaceOp::ADD)
	{
		// read number:
		op->status = table->Get(&transaction, op->key, data);
		
		if (op->status)
		{
			// parse number:
			num = strntoint64(data.buffer, data.length, &nread);
			if (nread == (unsigned) data.length)
			{
				num = num + op->num;
				 // print number:
				data.length = snwritef(data.buffer, data.size, "%I", num);
				 // write number:
				op->status &= table->Set(&transaction, op->key, data);
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
		op->status &= table->Get(&transaction, op->key, op->value);
		op->status &= table->Delete(&transaction, op->key);
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
