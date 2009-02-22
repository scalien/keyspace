#ifndef ASYNCDATABASE_H
#define ASYNCDATABASE_H

#include "MultiDatabaseOp.h"
#include "System/ThreadPool.h"
#include "System/Events/Callable.h"

class MultiDatabaseOp;

class AsyncDatabase
{
public:
	AsyncDatabase(int numThread);
	
	void		Add(MultiDatabaseOp* mdb);

private:
	ThreadPool	threadPool;
};

#endif
