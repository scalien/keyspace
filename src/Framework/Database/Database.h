#ifndef DATABASE_H
#define DATABASE_H

#include <db_cxx.h>
#include "System/ThreadPool.h"
#include "System/Events/Timer.h"
#include "System/Events/Callable.h"

class Table;
class Transaction;

class Database
{
	friend class Table;
	friend class Transaction;
public:
	Database();
	~Database();
	
	bool Init(const char *dbdir, int pageSize, int cacheSize);
	
	Table* GetTable(const char* name);
	
private:
	DbEnv			env;
	Table*			keyspace;
	Table*			test;
	ThreadPool		cpThread;
	bool			running;
	CdownTimer		checkpointTimeout;
	MFunc<Database>	onCheckpointTimeout;
	MFunc<Database>	checkpoint;

	void			OnCheckpointTimeout();
	void			Checkpoint();
};

// the global database object
extern Database database;

#endif
