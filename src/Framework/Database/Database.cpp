#include <string.h>

#include "Database.h"
#include "Table.h"

Database::Database() :
env(DB_CXX_NO_EXCEPTIONS)
{
	u_int32_t flags = DB_CREATE | DB_INIT_MPOOL;
	int mode = 0;
	const char* db_home = ".";
	
	env.open(db_home, flags, mode);
	
	state = new Table(this, "state");
	versionDB = new Table(this, "versionDB");
}

Database::~Database()
{
	delete versionDB;
	delete state;
	
	env.close(0);
}

Table* Database::GetTable(const char* name)
{
	if (strcmp(name, "state") == 0)
		return state;
	
	if (strcmp(name, "versionDB") == 0)
		return versionDB;
	
	return NULL;
}