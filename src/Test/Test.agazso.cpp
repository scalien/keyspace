#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Protocol/MemcacheServer.h"
#include "Application/Keyspace/Protocol/HttpServer.h"

#include "Framework/Transport/TransportTCPWriter.h"
#include "System/Time.h"

TransportTCPWriter writer;

void Write()
{
	ByteArray<100> hello;
	hello.Set("hello");

	writer.Write(hello);
}

int
main(int argc, char* argv[])
{
	IOProcessor*	ioproc;
	EventLoop*		eventloop;
	CFunc			writeCallback(&Write);
	CdownTimer		writeTimeout(1000, &writeCallback);
	
	ioproc = IOProcessor::New();
	eventloop = new EventLoop(ioproc);
	
	ioproc->Init();

//	KeyspaceDB kdb;
//	kdb.Init();
//	
//	MemcacheServer mcServer;
//	mcServer.Init(&kdb);
//	
//	HttpServer httpServer;
//	httpServer.Init(&kdb);

	Endpoint maro;
	maro.Set("192.168.1.240", 8080);
	
	writer.Init(ioproc, eventloop, maro);
	writer.Connect();
	
	eventloop->Add(&writeTimeout);
	eventloop->Run();	

	return 0;
}
