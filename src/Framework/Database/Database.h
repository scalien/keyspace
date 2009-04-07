#ifndef DATABASE_H
#define DATABASE_H

#include <db_cxx.h>

class Table;
class Transaction;

class Database
{
	friend class Table;
	friend class Transaction;
public:
	Database();
	~Database();
	
	bool Init(const char *dbdir);
	
	Table* GetTable(const char* name);
	
private:
	DbEnv env;
	Table* keyspace;
	Table* test;
};

// the global database object
extern Database database;

#endif
