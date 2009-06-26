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
	else if (op->IsList())
	{
		AsyncListVisitor *alv = new AsyncListVisitor(op);
		MultiDatabaseOp* mdbop = new AsyncMultiDatabaseOp();
		mdbop->Visit(table, *alv);
		dbReader.Add(mdbop);
	}
	else if (op->type == KeyspaceOp::SET)
	{
//		Log_Trace();
		op->status &= table->Set(&transaction, op->key, op->value);
//		Log_Trace();
		op->service->OnComplete(op);
	}
	else if (op->type == KeyspaceOp::TEST_AND_SET)
	{
		op->status &= table->Get(&transaction, op->key, data);
		if (data == op->test)
			op->status &= table->Set(&transaction, op->key, op->value);
		else
			op->value.Set(data);
		
		op->service->OnComplete(op);
	}
	else if (op->type == KeyspaceOp::ADD)
	{
		op->status = table->Get(&transaction, op->key, data); // read number
		
		if (op->status)
		{
			num = strntoint64(data.buffer, data.length, &nread); // parse number
			if (nread == (unsigned) data.length)
			{
				num = num + op->num;
				data.length = snwritef(data.buffer, data.size, "%I", num); // print number
				op->status &= table->Set(&transaction, op->key, data); // write number
				op->value.Allocate(data.length);
				op->value.Set(data);
			}
			else
				op->status = false;
		}
		op->service->OnComplete(op);
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
		op->status &= table->Get(&transaction, op->key, op->value);
		op->status &= table->Delete(&transaction, op->key);
		op->service->OnComplete(op);
	}

	else if (op->type == KeyspaceOp::PRUNE)
	{
		op->status &= table->Prune(&transaction, op->prefix);
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
