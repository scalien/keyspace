#ifndef TABLE_H
#define TABLE_H

#include "Database.h"
#include "System/Buffer.h"

class TableVisitor
{
public:
	virtual void Accept(const ByteString &key, const ByteString &value) = 0;
};

class Table
{
	friend class Transaction;
public:
	Table(Database* database, const char *name);
	~Table();
	
	bool Get(Transaction* transaction, const ByteString &key, ByteString &value);
	bool Get(Transaction* transaction, const char* key, ByteString &value);
	
	bool Put(Transaction* transaction, const ByteString &key, const ByteString &value);
	bool Put(Transaction* transaction, const char* key, const ByteString &value);
	bool Put(Transaction* transaction, const char* key, const char* value);
	
	bool Visit(TableVisitor &tv);
private:
	Database*	database;
	Db*			db;
};


#endif
