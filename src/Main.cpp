#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Application/TestDB/TestDB.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Protocol/HttpServer.h"
/*
#include "Framework/Transport/TransportTCPReader.h"
#include "Framework/Transport/TransportTCPWriter.h"

void f();

Endpoint me;
TransportTCPWriter writer;
CFunc cfunc(&f);
CdownTimer timer(1000, &cfunc);
ByteArray<8> bs;

void f()
{
	bs.Set("hello");

	writer.Write(bs);
	
	EventLoop::Get()->Add(&timer);
}
*/
int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("usage: %s <config-file>\n", argv[0]);
		return 1;
	}
	
	//Log_SetTimestamping(true);
	
	IOProcessor::Get()->Init();
	
	if (!PaxosConfig::Get()->Init(argv[1]))
		ASSERT_FAIL();

	ReplicatedLog::Get()->Init();
	
	KeyspaceDB kdb;
	kdb.Init();
	
//	TestDB testdb;
//	testdb.Init(ioproc, eventloop, &rl);

	HttpServer proto;
	proto.Init(&kdb);

	EventLoop::Get()->Run();
/*	

	IOProcessor::Get()->Init();
	
	TransportTCPReader reader;
	reader.Init(5000);
	
	me.Set("127.0.0.1", 5000);
	writer.Init(me);
	
	EventLoop::Get()->Add(&timer);
	
	EventLoop::Get()->Run();
*/
}
