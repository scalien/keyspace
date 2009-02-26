#ifndef KEYSPACEDB_H
#define KEYSPACEDB_H

#include "System/Containers/List.h"
#include "System/Buffer.h"
#include "System/Events/Callable.h"
#include "Framework/Database/Database.h"
#include "Framework/Database/Transaction.h"
#include "Framework/AsyncDatabase/MultiDatabaseOp.h"

#define MAX_VALUE_SIZE 64000

class KeyspaceOp;

class KeyspaceClient
{
public:
	virtual	void	OnComplete(KeyspaceOp* op, bool status) = 0;
};

class KeyspaceOp
{
public:
	enum Type
	{
		GET,
		SET,
		TEST_AND_SET
	};
	
	Type					type;
	ByteString				key;
	ByteString				value;
	ByteString				test;
	
	KeyspaceClient*			client;
};

class OpBuffers
{
public:
	char*					key;
	char*					value;
	char*					test;
};

class KeyspaceDB
{
public:
	KeyspaceDB();
	
	bool					Init();
	
	bool					Add(KeyspaceOp& op);	// the interface used by KeyspaceClient
	
	bool					StartDBOperation();
	
	void					OnDBComplete();
	MFunc<KeyspaceDB>		onDBComplete;
	
	DatabaseOp				MakeDatabaseOp(KeyspaceOp* keyspaceOp);
	
private:
	List<KeyspaceOp>		queuedOps;
	List<OpBuffers>			queuedOpBuffers;
	
	Table*					table;
	Transaction				transaction;
	MultiDatabaseOp			mdbop;
};

#endif
