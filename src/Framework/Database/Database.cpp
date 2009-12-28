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

static void DatabaseError(const DbEnv* /*dbenv*/,
						  const char* /*errpfx*/,
						  const char* msg)
{
	Log_Trace("%s", msg);
}

static void DatabaseTrace(const DbEnv* /*dbenv*/, const char* msg)
{
	Log_Trace("%s", msg);
}

Database::Database() :
env(DB_CXX_NO_EXCEPTIONS),
checkpoint(this, &Database::Checkpoint),
checkpointTimeout(&onCheckpointTimeout),
onCheckpointTimeout(this, &Database::OnCheckpointTimeout)
{
	cpThread = ThreadPool::Create(1);
}

Database::~Database()
{
	Shutdown();
}

bool Database::Init(const DatabaseConfig& config_)
{
	u_int32_t flags = DB_CREATE | DB_INIT_MPOOL |
	DB_INIT_TXN | DB_RECOVER | DB_THREAD | DB_PRIVATE;
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
#ifdef DB_VERB_FILEOPS
		env.set_verbose(DB_VERB_FILEOPS, 1);
#endif
#ifdef DB_VERB_FILEOPS_ALL
		env.set_verbose(DB_VERB_FILEOPS_ALL, 1);
#endif
#ifdef DB_VERB_RECOVERY
		env.set_verbose(DB_VERB_RECOVERY, 1);
#endif
#ifdef DB_VERB_REGISTER
		env.set_verbose(DB_VERB_REGISTER, 1);
#endif
#ifdef DB_VERB_REPLICATION
		env.set_verbose(DB_VERB_REPLICATION, 1);
#endif
#ifdef DB_VERB_WAITSFOR
		env.set_verbose(DB_VERB_WAITSFOR, 1);
#endif
	}
	
	Log_Trace();
	ret = env.open(config.dir, flags, mode);
	Log_Trace();
	if (ret != 0)
		return false;

#ifdef DB_LOG_AUTOREMOVE
	env.set_flags(DB_LOG_AUTOREMOVE, 1);
#else
	env.log_set_config(DB_LOG_AUTO_REMOVE, 1);
#endif
	
	keyspace = new Table(this, "keyspace", config.pageSize);

	Checkpoint();
	
	running = true;
	checkpointTimeout.SetDelay(config.checkpointTimeout);
	EventLoop::Add(&checkpointTimeout);
	cpThread->Start();
	
	return true;
}

void Database::Shutdown()
{
	if (!running)
		return;

	running = false;
	cpThread->Stop();
	delete keyspace;	
	env.close(0);
}

Table* Database::GetTable(const char* name)
{
	if (strcmp(name, "keyspace") == 0)
		return keyspace;
		
	return NULL;
}

void Database::OnCheckpointTimeout()
{
	cpThread->Execute(&checkpoint);
	EventLoop::Reset(&checkpointTimeout);
}

void Database::Checkpoint()
{
	int ret;

	Log_Trace("started");
	ret = env.txn_checkpoint(100*1000 /* in kilobytes */, 0, 0);
	if (ret < 0)
		ASSERT_FAIL();
	Log_Trace("finished");
}
