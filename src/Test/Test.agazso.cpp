#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Protocol/MemcacheServer.h"
#include "Application/Keyspace/Protocol/HttpServer.h"

int
main(int argc, char* argv[])
{
	IOProcessor*	ioproc;
	EventLoop*		eventloop;
	
	ioproc = IOProcessor::New();
	eventloop = new EventLoop(ioproc);
	
	ioproc->Init();

	KeyspaceDB kdb;
	kdb.Init();
	
	MemcacheServer mcServer;
	mcServer.Init(&kdb);
	
	HttpServer httpServer;
	httpServer.Init(&kdb);
	
	eventloop->Run();	

	return 0;
}
