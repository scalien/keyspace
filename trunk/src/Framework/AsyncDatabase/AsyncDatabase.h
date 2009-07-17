#ifndef ASYNCDATABASE_H
#define ASYNCDATABASE_H

#include "MultiDatabaseOp.h"
#include "System/ThreadPool.h"
#include "System/Events/Callable.h"

#define NUM_DB_WRITERS	1
#define NUM_DB_READERS	10

class MultiDatabaseOp;

class AsyncDatabase
{
public:
	AsyncDatabase(int numThread);
	
	void		Shutdown();

	void		Add(MultiDatabaseOp* dbop);

private:
	ThreadPool	threadPool;
};


// globals
extern AsyncDatabase dbWriter;
extern AsyncDatabase dbReader;

#endif
