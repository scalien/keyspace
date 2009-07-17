#include "AsyncDatabase.h"
#include "MultiDatabaseOp.h"


// globals
AsyncDatabase dbWriter(NUM_DB_WRITERS);
AsyncDatabase dbReader(NUM_DB_READERS);


AsyncDatabase::AsyncDatabase(int numThread) :
threadPool(numThread)
{
	threadPool.Start();
}

void AsyncDatabase::Shutdown()
{
	threadPool.Stop();
}

void AsyncDatabase::Add(MultiDatabaseOp* dbop)
{
	dbop->active = true;
	
	threadPool.Execute(dbop->GetOperation());
}
