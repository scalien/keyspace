#ifndef ASYNCDATABASEOP_H
#define ASYNCDATABASEOP_H

#include "System/Buffer.h"
#include "System/Events/Callable.h"

class Table;
class TableVisitor;
class Transaction;
class ByteString;
class AsyncDatabase;

class DatabaseOp
{
public:
	enum Type {
		GET,
		SET,
		VISIT
	};
	
	Type				type;
	Table*				table;
	ByteString			key;
	ByteString			value;
	TableVisitor*		visitor;
	bool				ret;
};

class MultiDatabaseOp
{
friend class AsyncDatabase;
typedef MFunc<MultiDatabaseOp> Func;

public:
	MultiDatabaseOp();
	
	void			Init();

	bool			Get(Table* table, const ByteString& key, ByteString& value);
	bool			Set(Table* table, const ByteString& key, ByteString& value);
	bool			Set(Table* table, const char* key, ByteString &value);
	bool			Set(Table* table, const char* key, const char* value);
	bool			Visit(Table* table, TableVisitor &tv);
	bool			Add(DatabaseOp& op);

	void			SetTransaction(Transaction* tx = 0);

	void			SetCallback(Callable* userCallback);
	Callable*		GetOperation();
	ByteString*		GetValue(int i);
	bool			GetReturnValue(int i);
	int				GetNumOp();
	bool			IsActive() { return active; }
	
private:
	bool			active;
	DatabaseOp		ops[1024];
	size_t			numop;
	
	Callable*		userCallback;
	Transaction*	tx;
	
	void			OnComplete();
	Func			onComplete;
	
	void			Operation();
	Func			operation;
};
										

#endif
