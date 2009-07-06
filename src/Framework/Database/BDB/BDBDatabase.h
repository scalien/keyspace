#ifndef BDBDATABASE_H
#define BDBDATABASE_H

#include <db_cxx.h>
#include "System/Common.h"
#include "System/ThreadPool.h"
#include "System/Events/Timer.h"
#include "System/Events/Callable.h"

#define DATABASE_CONFIG_DIR					"."
#define DATABASE_CONFIG_PAGE_SIZE			4096
#define DATABASE_CONFIG_CACHE_SIZE			100*MB
#define DATABASE_CONFIG_LOG_BUFFER_SIZE		2*MB
#define DATABASE_CONFIG_LOG_MAX_FILE		0
#define DATABASE_CONFIG_CHECKPOINT_TIMEOUT	60000
#define DATABASE_CONFIG_VERBOSE				false

class DatabaseConfig
{
public:
	DatabaseConfig()
	{
		dir = DATABASE_CONFIG_DIR;
		pageSize = DATABASE_CONFIG_PAGE_SIZE;
		cacheSize = DATABASE_CONFIG_CACHE_SIZE;
		logBufferSize = DATABASE_CONFIG_LOG_BUFFER_SIZE;
		logMaxFile = DATABASE_CONFIG_LOG_MAX_FILE;
		checkpointTimeout = DATABASE_CONFIG_CHECKPOINT_TIMEOUT;
		verbose = DATABASE_CONFIG_VERBOSE;
	}
	
	const char*	dir;
	int			pageSize;
	int			cacheSize;
	int			logBufferSize;
	int			logMaxFile;
	int			checkpointTimeout;
	bool		verbose;
};

class Table;
class Transaction;

class Database
{
	friend class Table;
	friend class Transaction;
public:
	Database();
	virtual ~Database();
	
	virtual bool	Init(const DatabaseConfig& config);
	
	virtual Table*	GetTable(const char* name);
	
	virtual void	OnCheckpointTimeout();
	virtual void	Checkpoint();

	
private:
	DatabaseConfig	config;
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
