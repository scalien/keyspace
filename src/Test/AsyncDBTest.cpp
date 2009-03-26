#include "test.h"
#include "System/Buffer.h"
#include "System/IO/IOProcessor.h"
#include "System/Events/EventLoop.h"
#include "Framework/Database/Database.h"
#include "Framework/Database/Transaction.h"
#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "Time.h"

Database db;
AsyncDatabase writer(1);
MultiDatabaseOp dbop;
long start;

#define NUM		1
#define NUMPUT	10000

void cb()
{
	TEST_LOG("callback");
	
	long elapsed = Now() - start;
	printf("%d transactions took %d msec *** %f tps\n",
		NUM, elapsed, (double)1000*NUM/elapsed);
	
}

CFunc callable(&cb);

int test()
{
	IOProcessor* ioproc = IOProcessor::Get();
	EventLoop* eventloop = new EventLoop(ioproc);
	ioproc->Init();
	
	
	Table* table = db.GetTable("test");
	Transaction tx(table);
	
	ByteArray<32> key("hol");
	ByteArray<32> value("peru");
	
	dbop.SetCallback(&callable);
	
	for (int j = 0; j < NUMPUT; j++)
	{
		bool ret = dbop.Put(table, key, value);
		if (!ret)
			TEST_LOG("false");
	}
	
	dbop.SetTransaction(&tx);
	
	start = Now();
	
	writer.Add(&dbop);
	
	eventloop->Run();
	
	return TEST_SUCCESS;
}

TEST_MAIN(test);
