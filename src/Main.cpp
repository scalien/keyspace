#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Application/TestDB/TestDB.h"

int main(int argc, char* argv[])
{
	IOProcessor*	ioproc;
	EventLoop*	eventloop;
	
	if (argc != 2)
	{
		printf("usage: %s <config-file>\n", argv[0]);
		return 1;
	}
	
	ioproc = IOProcessor::New();
	eventloop = new EventLoop(ioproc);
	
	ioproc->Init();

	ReplicatedLog rl;
	rl.Init(ioproc, eventloop, argv[1]);
	
	TestDB testdb;
	testdb.Init(ioproc, eventloop, &rl);
	
	eventloop->Run();	
}