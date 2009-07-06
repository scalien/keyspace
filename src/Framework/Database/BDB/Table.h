#ifndef TABLE_H
#define TABLE_H

#include "Database.h"
#include "Cursor.h"
#include "System/Buffer.h"

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
	Table(Database* database, const char *name, int pageSize = 0);
	~Table();
	
	bool		Iterate(Transaction* transaction, Cursor& cursor);
	
	bool		Get(Transaction* transaction, const ByteString &key, ByteString &value);
	bool		Get(Transaction* transaction, const char* key, ByteString &value);
	bool		Get(Transaction* transaction, const char* key, uint64_t &value);
	
	bool		Set(Transaction* transaction, const ByteString &key, const ByteString &value);
	bool		Set(Transaction* transaction, const char* key, const ByteString &value);
	bool		Set(Transaction* transaction, const char* key, const char* value);
	
	bool		Delete(Transaction* transaction, const ByteString &key);
	bool		Prune(Transaction* transaction, const ByteString &prefix);
	bool		Truncate(Transaction* transaction = NULL);
	
	bool		Visit(TableVisitor &tv);
	
private:
	Database*	database;
	Db*			db;
};


#endif
