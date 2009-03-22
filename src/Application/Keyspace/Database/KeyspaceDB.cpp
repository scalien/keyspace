#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "KeyspaceDB.h"
#include <assert.h>
#include <stdlib.h>
#include "System/Log.h"

KeyspaceDB::KeyspaceDB()
: onDBComplete(this, &KeyspaceDB::OnDBComplete)
{
}

bool KeyspaceDB::Init()
{
	Log_Trace();

	mdbop.SetCallback(&onDBComplete);
	
	table = database.GetTable("test");
	
	transaction.Set(table);
	
	return true;
}

bool KeyspaceDB::Add(KeyspaceOp& op_)
{
	Log_Trace();
	
	KeyspaceOp op = op_;

	assert(queuedOps.Size() == queuedOpBuffers.Size());

	assert(op.type != KeyspaceOp::TEST_AND_SET);
	
	assert(table != NULL);


	OpBuffers opBuffers;
	
	// allocate and copy buffers for the values
	
	opBuffers.key =	(char*) Alloc(op.key.length);
	op.key.size = op.key.length;
	memcpy(opBuffers.key, op.key.buffer, op.key.length);
	op.key.buffer = opBuffers.key;
	
	if (op.type == KeyspaceOp::GET)
	{
		opBuffers.value = (char*) Alloc(MAX_VALUE_SIZE);
		op.value.size = MAX_VALUE_SIZE;
	}
	else
	{
		opBuffers.value = (char*) Alloc(op.value.length);
		op.value.size = op.value.length;
	}
	memcpy(opBuffers.value, op.value.buffer, op.value.length);
	op.value.buffer = opBuffers.value;
	
	opBuffers.test = (char*) Alloc(op.test.length);
	op.test.size = op.test.length;
	memcpy(opBuffers.test, op.test.buffer, op.test.length);
	op.test.buffer = opBuffers.test;

	// enqueue
	
	queuedOps.Append(op);
	queuedOpBuffers.Append(opBuffers);
	
	// if we're currently not waiting on a db op
	// to complete, we may as well start one
	
	if (!mdbop.active)
		StartDBOperation();
	
	return true;
}

DatabaseOp KeyspaceDB::MakeDatabaseOp(KeyspaceOp* keyspaceOp)
{
	DatabaseOp dbop;

	if (keyspaceOp->type == KeyspaceOp::GET)
		dbop.type = DatabaseOp::GET;
	if (keyspaceOp->type == KeyspaceOp::SET)
		dbop.type = DatabaseOp::SET;
	
	dbop.table = table;
	dbop.key = keyspaceOp->key;
	dbop.value = keyspaceOp->value;
	
	return dbop;
}

bool KeyspaceDB::StartDBOperation()
{
	Log_Trace();

	assert(queuedOps.Size() == queuedOpBuffers.Size());

	KeyspaceOp* it;
	DatabaseOp dbop;
	bool ret;
	
	mdbop.Init();
	
	for (it = queuedOps.Head(); it != NULL; it = queuedOps.Next(it))
	{
		dbop = MakeDatabaseOp(it);

		ret = mdbop.Add(dbop);

		if (!ret)
			break;		// MultiDatabaseOp is full
	}
	
	if (mdbop.GetNumOp() > 0)
	{
		mdbop.SetTransaction(&transaction);
		dbWriter.Add(&mdbop); // I'm using the dbWriter even though it may contain reads
	}
	
	return true;
}

void KeyspaceDB::OnDBComplete()
{
	Log_Trace();
	
	int			i, numOp;
	KeyspaceOp*	opit;
	OpBuffers*	bfit;
	
	numOp = mdbop.GetNumOp();

	assert(numOp <= queuedOps.Size());
	assert(numOp <= queuedOpBuffers.Size());
	
	opit = queuedOps.Head();
	bfit = queuedOpBuffers.Head();
	
	for (i = 0; i < numOp; i++)
	{
		bool ret = mdbop.GetReturnValue(i);

		if (ret)
			opit->value = *mdbop.GetValue(i);

		// call client's callback
		
		opit->client->OnComplete(opit, ret);
		
		// free buffers allocated in Add()

		free(bfit->key);
		free(bfit->value);
		free(bfit->test);
		
		// remove from queue
		
		opit = queuedOps.Remove(opit);
		bfit = queuedOpBuffers.Remove(bfit);
	}
	
	// start a new operation
	
	if (queuedOps.Size() > 0)
		StartDBOperation();
}

