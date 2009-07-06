#ifndef BDBTABLE_H
#define BDBTABLE_H

#include "System/Buffer.h"

class Cursor;
class Database;
class Db;

class TableVisitor
{
public:
	virtual	~TableVisitor() {}
	virtual bool Accept(const ByteString &key, const ByteString &value) = 0;
	virtual const ByteString* GetStartKey() { return 0; }
	virtual void OnComplete() {}
};

class Table
{
	friend class Transaction;
public:
	Table(const char *name, int pageSize = 0);
	virtual ~Table();
	
	virtual bool	Iterate(Transaction* transaction, Cursor& cursor);
	
	virtual bool	Get(Transaction* transaction, const ByteString &key, ByteString &value);
	bool Get(Transaction* transaction, const char* key, ByteString &value);
	bool Get(Transaction* transaction, const char* key, uint64_t& value);
	
	virtual bool	Set(Transaction* transaction, const ByteString &key, const ByteString &value);
	bool Set(Transaction* transaction, const char* key, const ByteString &value);
	bool Set(Transaction* transaction, const char* key, const char* value);
	
	virtual bool	Delete(Transaction* transaction, const ByteString &key);
	virtual bool	Prune(Transaction* transaction, const ByteString &prefix);
//	virtual bool	Truncate(Transaction* transaction = NULL);
	
	virtual bool	Visit(TableVisitor &tv);
	
private:
	Db*			db;
};


#endif
