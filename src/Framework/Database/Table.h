#ifndef TABLE_H
#define TABLE_H

#include "Database.h"
#include "Cursor.h"
#include "System/Buffer.h"

class Transaction;

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
public:
	virtual		~Table() {}
	
	virtual bool	Iterate(Transaction* transaction, Cursor& cursor) = 0;
	
	virtual bool	Get(Transaction* transaction, const ByteString &key, ByteString &value) = 0;
	bool			Get(Transaction* transaction, const char* key, ByteString &value);
	bool			Get(Transaction* transaction, const char* key, uint64_t &value);
	
	virtual bool	Set(Transaction* transaction, const ByteString &key, const ByteString &value) = 0;
	bool			Set(Transaction* transaction, const char* key, const ByteString &value);
	bool			Set(Transaction* transaction, const char* key, const char* value);
	
	virtual bool	Delete(Transaction* transaction, const ByteString &key) = 0;
	virtual bool	Prune(Transaction* transaction, const ByteString &prefix) = 0;
	virtual bool	Truncate(Transaction* transaction = NULL) = 0;
	
	virtual bool	Visit(TableVisitor &tv) = 0;
};


#endif
