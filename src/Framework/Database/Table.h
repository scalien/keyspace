#ifndef TABLE_H
#define TABLE_H

#include "Database.h"
#include "Buffer.h"

class Table
{
	friend class Transaction;
public:
	Table(Database* database, const char *name);
	~Table();
	
	bool Get(Transaction* transaction, const ByteString &key, ByteString &value);
	
	bool Put(Transaction* transaction, const ByteString &key, const ByteString &value);
	bool Put(Transaction* transaction, char* key, const ByteString &value);
	bool Put(Transaction* transaction, char* key, char* value);
	
private:
	Database*	database;
	Db*			db;
};


#endif
