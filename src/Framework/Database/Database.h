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
	
	bool	Init(const char *dbdir, int pageSize, int cacheSize, int logBufferSize);
	
	Table*	GetTable(const char* name);
	
	void	OnCheckpointTimeout();
	void	Checkpoint();

	
private:
	DbEnv			env;
	Table*			keyspace;
	ThreadPool		cpThread;
	bool			running;
	MFunc<Database>	checkpoint;
	CdownTimer		checkpointTimeout;
	MFunc<Database>	onCheckpointTimeout;
};

// the global database object
extern Database database;

#endif
