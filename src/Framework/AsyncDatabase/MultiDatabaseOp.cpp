#include "MultiDatabaseOp.h"
#include "System/Buffer.h"
#include "System/ThreadPool.h"
#include "System/IO/IOProcessor.h"
#include "Framework/Database/Table.h"
#include "Framework/Database/Transaction.h"

MultiDatabaseOp::MultiDatabaseOp() :
active(false),
onComplete(this, &MultiDatabaseOp::OnComplete),
operation(this, &MultiDatabaseOp::Operation)
{
	Init();
}


void MultiDatabaseOp::Init()
{
	numop = 0;
	//userCallback = NULL;
	tx = NULL;
}


bool MultiDatabaseOp::Get(Table* table, const ByteString &key, ByteString& value)
{
	DatabaseOp* op;
	
	if (numop >= SIZE(ops))
		return false;
	
	op = &ops[numop];
	op->type = DatabaseOp::GET;
	op->table = table;
	op->key = key;
	op->value = value;
	
	numop++;
	
	return true;
}


bool MultiDatabaseOp::Set(Table* table, const ByteString& key, ByteString& value)
{
	DatabaseOp* op;

	if (numop >= SIZE(ops))
		return false;
	
	op = &ops[numop];
	op->type = DatabaseOp::SET;
	op->table = table;
	op->key = key;
	op->value = value;
	
	numop++;
	
	return true;
}

bool MultiDatabaseOp::Set(Table* table, char* key, ByteString &value)
{
	int len;
	
	len = strlen(key);
	const ByteString bsKey(len, len, key);
	
	return MultiDatabaseOp::Set(table, bsKey, value);
}

bool MultiDatabaseOp::Set(Table* table, char* key, char* value)
{
	int len;
	
	len = strlen(key);
	ByteString bsKey(len, len, key);
	
	len = strlen(value);
	ByteString bsValue(len, len, value);
	
	return MultiDatabaseOp::Set(table, bsKey, bsValue);
}

bool MultiDatabaseOp::Visit(Table* table, TableVisitor& tv)
{
	DatabaseOp* op;

	if (numop >= SIZE(ops))
		return false;
	
	op = &ops[numop];
	op->type = DatabaseOp::VISIT;
	op->table = table;
	op->visitor = &tv;
	
	numop++;
	
	return true;
}

bool MultiDatabaseOp::Add(DatabaseOp& op)
{
	if (numop >= SIZE(ops))
		return false;
	
	ops[numop] = op;
	numop++;
	
	return true;
}

void MultiDatabaseOp::SetTransaction(Transaction* tx)
{
	this->tx = tx;
}

void MultiDatabaseOp::SetCallback(Callable* userCallback_)
{
	this->userCallback = userCallback_;
}

Callable* MultiDatabaseOp::GetOperation()
{
	return &operation;
}

const ByteString* MultiDatabaseOp::GetKey(int i)
{
	return &ops[i].key;
}


ByteString*	MultiDatabaseOp::GetValue(int i)
{
	return &ops[i].value;
}


bool MultiDatabaseOp::GetReturnValue(int i)
{
	return ops[i].ret;
}


int MultiDatabaseOp::GetNumOp()
{
	return numop;
}


void MultiDatabaseOp::Operation()
{
	size_t		i;
	DatabaseOp* op;
	
	if (tx && !tx->IsActive())
		tx->Begin();
		
	for (i = 0; i < numop; i++)
	{
		op = &ops[i];
		if (op->type == DatabaseOp::GET)
			op->ret = op->table->Get(tx, op->key, op->value);
		else if (op->type == DatabaseOp::SET)
			op->ret = op->table->Set(tx, op->key, op->value);
		else if (op->type == DatabaseOp::VISIT)
			op->ret = op->table->Visit(*op->visitor);
	}
	
	if (tx)
		tx->Commit();
		
	IOProcessor::Complete(&onComplete);
}

void MultiDatabaseOp::OnComplete()
{
	active = false;
	
	Call(userCallback);
}
