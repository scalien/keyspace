#include "AsyncDatabase.h"
#include "MultiDatabaseOp.h"


// globals
AsyncDatabase dbWriter;
AsyncDatabase dbReader;


void AsyncDatabase::Init(int numThread)
{
	threadPool = ThreadPool::Create(numThread);
	threadPool->Start();
}

void AsyncDatabase::Shutdown()
{
	threadPool->Stop();
	delete threadPool;
}

void AsyncDatabase::Add(MultiDatabaseOp* dbop)
{
	dbop->active = true;
	
	threadPool->Execute(dbop->GetOperation());
}
