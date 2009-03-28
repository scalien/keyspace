#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Application/TestDB/TestDB.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Protocol/HttpServer.h"

int main(int argc, char* argv[])
{
	IOProcessor*	ioproc;
	EventLoop*	eventloop;
	
	if (argc != 2)
	{
		printf("usage: %s <config-file>\n", argv[0]);
		return 1;
	}
	
	//Log_SetTimestamping(true);
	
	ioproc = IOProcessor::Get();
	eventloop = new EventLoop(ioproc);
	
	if (!PaxosConfig::Get()->Init(argv[1]))
		ASSERT_FAIL();
	
	ioproc->Init();

	ReplicatedLog rl;
	rl.Init(ioproc, eventloop);
	
	KeyspaceDB kdb;
	kdb.Init(ioproc, &rl);
	
//	TestDB testdb;
//	testdb.Init(ioproc, eventloop, &rl);

	HttpServer proto;
	proto.Init(&kdb);

	eventloop->Run();
}
