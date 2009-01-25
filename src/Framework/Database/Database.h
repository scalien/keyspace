#ifndef DATABASE_H
#define DATABASE_H

#include <db_cxx.h>

class Database
{
	friend class Table;
public:
	Database();
	~Database();
	
	Table * GetTable(const char *name);
private:
	DbEnv env;
	Table *state;
	Table *versionDB;
};

#endif
