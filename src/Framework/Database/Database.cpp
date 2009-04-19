#include <string.h>

#include "Database.h"
#include "Table.h"

// the global database
Database database;

Database::Database() :
env(DB_CXX_NO_EXCEPTIONS)
{
}

bool Database::Init(const char* dbdir)
{
	u_int32_t flags = DB_CREATE | DB_INIT_MPOOL | DB_INIT_TXN | DB_RECOVER_FATAL |
					  DB_LOG_AUTOREMOVE/* | DB_THREAD*/;
	int mode = 0;
	int ret;
	
	ret = env.open(dbdir, flags, mode);
	if (ret != 0)
	{
		return false;
	}
	
	keyspace = new Table(this, "keyspace");
	test = new Table(this, "test");
	
	return true;
}

Database::~Database()
{
	delete keyspace;
	delete test;
	
	env.close(0);
}

Table* Database::GetTable(const char* name)
{
	if (strcmp(name, "keyspace") == 0)
		return keyspace;
		
	if (strcmp(name, "test") == 0)
		return test;
		
	return NULL;
}
