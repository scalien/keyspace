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
	virtual const ByteString* GetKeyHint() { return 0; }
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
	
	bool		Set(Transaction* transaction, const ByteString &key, const ByteString &value);
	bool		Set(Transaction* transaction, const char* key, const ByteString &value);
	bool		Set(Transaction* transaction, const char* key, const char* value);
	
	bool		Delete(Transaction* transaction, const ByteString &key);
	
	bool		Visit(TableVisitor &tv);
	
	bool		Truncate(Transaction* transaction = NULL);
private:
	Database*	database;
	Db*			db;
};


#endif
