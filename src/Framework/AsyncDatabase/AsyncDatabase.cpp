#include "AsyncDatabase.h"
#include "MultiDatabaseOp.h"


AsyncDatabase::AsyncDatabase(int numThread) :
threadPool(numThread)
{
	threadPool.Start();
}

void AsyncDatabase::Add(MultiDatabaseOp* mdbOp)
{
	threadPool.Execute(mdbOp->GetOperation());
}
