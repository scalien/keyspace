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

Database::Database() :
env(DB_CXX_NO_EXCEPTIONS),
cpThread(1),
checkpoint(this, &Database::Checkpoint),
onCheckpointTimeout(this, &Database::OnCheckpointTimeout),
checkpointTimeout(&onCheckpointTimeout)
{
}

Database::~Database()
{
	running = false;
	cpThread.Stop();
	delete keyspace;
	delete test;

	env.close(0);
}

bool Database::Init(const char* dbdir, int pageSize, int cacheSize)
{
	u_int32_t flags = DB_CREATE | DB_INIT_MPOOL | DB_INIT_TXN | DB_RECOVER_FATAL /* | DB_THREAD*/;
	int mode = 0;
	int ret;
	
	if (cacheSize != 0)
	{
		u_int32_t gbytes = cacheSize / (1024 * 1024 * 1024);
		u_int32_t bytes = cacheSize % (1024 * 1024 * 1024);
		
		//env.set_cache_max(gbytes, bytes);
		env.set_cachesize(gbytes, bytes, 4);
	}

	ret = env.open(dbdir, flags, mode);
	if (ret != 0)
		return false;

	env.set_flags(DB_LOG_AUTOREMOVE, 1);
	
	keyspace = new Table(this, "keyspace", pageSize);
	test = new Table(this, "test", pageSize);
	
	running = true;
	cpThread.Start();
	EventLoop::Add(&checkpointTimeout);
	
	return true;
}

Table* Database::GetTable(const char* name)
{
	if (strcmp(name, "keyspace") == 0)
		return keyspace;
		
	if (strcmp(name, "test") == 0)
		return test;
		
	return NULL;
}

void Database::OnCheckpointTimeout()
{
	cpThread.Execute(&checkpoint);
}

void Database::Checkpoint()
{
	int ret;

	Log_Trace();
	ret = env.txn_checkpoint(100000, 0, 0);
	if (ret < 0)
		ASSERT_FAIL();
}
