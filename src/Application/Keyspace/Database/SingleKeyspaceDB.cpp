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
	Log_Trace();
	
	bool			ret, isWrite;
	int64_t			num;
	unsigned		nread;
	
	isWrite = op->IsWrite();
	
	if (isWrite && !transaction.IsActive())
	{
		transaction.Set(table);
		transaction.Begin();
	}
	
	ret = true;
	if (op->IsWrite() && writePaxosID)
	{
		if (table->Set(&transaction, "@@paxosID", "1"))
			writePaxosID = false;
	}
	
	if (op->IsGet())
	{
		op->value.Allocate(KEYSPACE_VAL_SIZE);
		ret &= table->Get(NULL, op->key, op->value);
		op->service->OnComplete(op, ret);
	}
	else if (op->IsList())
	{
		AsyncListVisitor *alv = new AsyncListVisitor(op->prefix, op, op->count);
		MultiDatabaseOp* mdbop = new AsyncMultiDatabaseOp();
		mdbop->Visit(table, *alv);
		dbReader.Add(mdbop);
	}
	else if (op->type == KeyspaceOp::SET)
	{
		Log_Trace();
		ret &= table->Set(&transaction, op->key, op->value);
		Log_Trace();
		op->service->OnComplete(op, ret);
	}
	else if (op->type == KeyspaceOp::TEST_AND_SET)
	{
		ret &= table->Get(&transaction, op->key, data);
		if (data == op->test)
		{
			ret &= table->Set(&transaction, op->key, op->value);
		}
		else
		{
			op->value.Reallocate(data.length);
			op->value.Set(data);
		}
		
		op->service->OnComplete(op, ret);
	}
	else if (op->type == KeyspaceOp::ADD)
	{
		ret = table->Get(&transaction, op->key, data); // read number
		
		if (ret)
		{
			num = strntoint64_t(data.buffer, data.length, &nread); // parse number
			if (nread == (unsigned) data.length)
			{
				num = num + op->num;
				data.length = snwritef(data.buffer, data.size, "%I", num); // print number
				ret &= table->Set(&transaction, op->key, data); // write number
				op->value.Allocate(data.length);
				op->value.Set(data);
			}
			else
				ret = false;
		}
		op->service->OnComplete(op, ret);
	}
	else if (op->type == KeyspaceOp::DELETE)
	{
		ret &= table->Delete(&transaction, op->key);
		op->service->OnComplete(op, ret);
	}
	else if (op->type == KeyspaceOp::PRUNE)
	{
		ret &= table->Prune(&transaction, op->prefix);
		op->service->OnComplete(op, ret);
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
