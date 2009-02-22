#ifndef ASYNCDATABASEOP_H
#define ASYNCDATABASEOP_H

#include "System/Events/Callable.h"

class Table;
class Transaction;
class ByteString;
class ThreadPool;

class DatabaseOp
{
public:
	enum Operation {
		GET,
		PUT
	};
	
	Operation			op;
	Table*				table;
	const ByteString*	key;
	ByteString*			value;
	bool				ret;
};

class MultiDatabaseOp
{
public:
	MultiDatabaseOp();
	
	void					Init();
	bool					Get(Table* table, const ByteString& key, ByteString& value);
	bool					Put(Table* table, const ByteString& key, ByteString& value);
	void					SetTransaction(Transaction* tx = 0);
	void					SetCallback(Callable* onComplete);
	Callable*				GetOperation();
	const ByteString*		GetKey(int i);
	ByteString*				GetValue(int i);
	bool					GetReturnValue(int i);
	int						GetNumOp();
	bool					IsActive();
	
private:
	DatabaseOp				ops[1024];
	int						numop;
	Callable*				onComplete;
	MFunc<MultiDatabaseOp>	operation;
	Transaction*			tx;
	
	void					Operation();
};
										

#endif
