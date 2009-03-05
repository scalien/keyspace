#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Protocol/MemcacheServer.h"
#include "Application/Keyspace/Protocol/HttpServer.h"

int main(int argc, char* argv[])
{
	IOProcessor*	ioproc;
	EventLoop*	eventloop;
	
/*
	if (argc != 2)
	{
		printf("usage: %s <config-file>\n", argv[0]);
		return 1;
	}
*/	
	ioproc = IOProcessor::New();
	eventloop = new EventLoop(ioproc);
	
	ioproc->Init();

	KeyspaceDB kdb;
	kdb.Init();
	
	MemcacheServer mcache;
	mcache.Init(&kdb);
	
	HttpServer httpServer;
	httpServer.Init(&kdb);
	
	eventloop->Run();	
}