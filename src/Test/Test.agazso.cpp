#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Protocol/MemcacheServer.h"
#include "Application/Keyspace/Protocol/HttpServer.h"

#include "Framework/Transport/TransportUDPWriter.h"
#include "System/Time.h"

void Write();

TransportUDPWriter	writer;
CFunc				writeCallback(&Write);
CdownTimer			writeTimeout(1000, &writeCallback);
EventLoop*			eventloop;

void Write()
{
	ByteArray<100> hello;
	hello.Set("hello");

	writer.Write(hello);
	eventloop->Reset(&writeTimeout);
}

int
main(int argc, char* argv[])
{
	IOProcessor*	ioproc;
	
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
	
	eventloop->Add(&writeTimeout);
	eventloop->Run();	

	return 0;
}
