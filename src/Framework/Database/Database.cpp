#include <string.h>

#include "System/Events/Callable.h"
#include "System/Events/EventLoop.h"
#include "System/Time.h"
#include "System/Log.h"

#include "Database.h"
#include "Table.h"

#define DATABASE_DEFAULT_CACHESIZE	(256*1024)

// the global database
Database database;

static void DatabaseError(const DbEnv* /*dbenv*/, const char* /*errpfx*/, const char* msg)
{
	Log_Trace("%s", msg);
}

static void DatabaseTrace(const DbEnv* /*dbenv*/, const char* msg)
{
	Log_Trace("%s", msg);
}

Database::Database() :
env(DB_CXX_NO_EXCEPTIONS),
cpThread(1),
checkpoint(this, &Database::Checkpoint),
checkpointTimeout(&onCheckpointTimeout),
onCheckpointTimeout(this, &Database::OnCheckpointTimeout)
{
}

Database::~Database()
{
	running = false;
	cpThread.Stop();
	delete keyspace;

	env.close(0);
}

bool Database::Init(const DatabaseConfig& config_)
{
	u_int32_t flags = DB_CREATE | DB_INIT_MPOOL |
	DB_INIT_TXN | DB_RECOVER_FATAL | DB_THREAD;
	int mode = 0;
	int ret;
	
	env.set_errcall(DatabaseError);
	env.set_msgcall(DatabaseTrace);
	
	config = config_;
	
	if (config.cacheSize != 0)
	{
		u_int32_t gbytes = config.cacheSize / (1024 * 1024 * 1024);
		u_int32_t bytes = config.cacheSize % (1024 * 1024 * 1024);
		
		//env.set_cache_max(gbytes, bytes);
		env.set_cachesize(gbytes, bytes, 4);
	}

	if (config.logBufferSize != 0)
		env.set_lg_bsize(config.logBufferSize);
	
	if (config.logMaxFile != 0)
		env.set_lg_max(config.logMaxFile);
		
	if (config.verbose)
	{
		env.set_msgcall(DatabaseTrace);
		env.set_verbose(DB_VERB_FILEOPS, 1);
		env.set_verbose(DB_VERB_FILEOPS_ALL, 1);
		env.set_verbose(DB_VERB_RECOVERY, 1);
		env.set_verbose(DB_VERB_REGISTER, 1);
		env.set_verbose(DB_VERB_REPLICATION, 1);
		env.set_verbose(DB_VERB_WAITSFOR, 1);
	}
	
	ret = env.open(config.dir, flags, mode);
	if (ret != 0)
		return false;

	env.set_flags(DB_LOG_AUTOREMOVE, 1);
	
	keyspace = new Table(this, "keyspace", config.pageSize);

	Checkpoint();
	
	running = true;
	checkpointTimeout.SetDelay(config.checkpointTimeout);
	EventLoop::Add(&checkpointTimeout);
	cpThread.Start();
		
	return true;
}

Table* Database::GetTable(const char* name)
{
	if (strcmp(name, "keyspace") == 0)
		return keyspace;
		
	return NULL;
}

void Database::OnCheckpointTimeout()
{
	cpThread.Execute(&checkpoint);
	EventLoop::Reset(&checkpointTimeout);
}

void Database::Checkpoint()
{
	int ret;

	Log_Trace();
	ret = env.txn_checkpoint(100000, 0, 0);
	if (ret < 0)
		ASSERT_FAIL();
}
