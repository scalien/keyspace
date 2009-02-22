#include "AsyncDatabase.h"
#include "MultiDatabaseOp.h"


AsyncDatabase::AsyncDatabase(int numThread) :
threadPool(numThread)
{
}

void AsyncDatabase::Add(MultiDatabaseOp* mdbOp)
{
	threadPool.Execute(mdbOp->GetOperation());
}
