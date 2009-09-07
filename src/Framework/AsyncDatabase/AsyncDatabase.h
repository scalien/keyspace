#ifndef ASYNCDATABASE_H
#define ASYNCDATABASE_H

#include "MultiDatabaseOp.h"
#include "System/ThreadPool.h"
#include "System/Events/Callable.h"

class MultiDatabaseOp;

class AsyncDatabase
{
public:
	void		Init(int numThread);
	void		Shutdown();

	void		Add(MultiDatabaseOp* dbop);

private:
	ThreadPool*	threadPool;
};


// globals
extern AsyncDatabase dbWriter;
extern AsyncDatabase dbReader;

#endif
