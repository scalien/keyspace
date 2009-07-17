#ifndef DATABASE_H
#define DATABASE_H

#include <db_cxx.h>
#include "System/Common.h"
#include "System/ThreadPool.h"
#include "System/Events/Timer.h"
#include "System/Events/Callable.h"
#include "DatabaseConfig.h"

class Table;
class Transaction;

class Database
{
	friend class Table;
	friend class Transaction;
	typedef MFunc<Database> Func;

public:
	Database();
	~Database();
	
	bool			Init(const DatabaseConfig& config);
	void			Shutdown();
	
	Table*			GetTable(const char* name);
	
	void			OnCheckpointTimeout();
	void			Checkpoint();

private:
	DatabaseConfig	config;
	DbEnv			env;
	Table*			keyspace;
	ThreadPool		cpThread;
	bool			running;
	Func			checkpoint;
	CdownTimer		checkpointTimeout;
	Func			onCheckpointTimeout;
};

// global
extern Database database;

#endif
