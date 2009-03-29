#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Protocol/MemcacheServer.h"
#include "Application/Keyspace/Protocol/HttpServer.h"

#include "Framework/Transport/TransportUDPWriter.h"
#include "System/Time.h"


void Write();

TransportUDPWriter	writer;
CFunc				writeCallback(&Write);
CdownTimer			writeTimeout(1000, &writeCallback);

void Write()
{
	ByteArray<100> hello;
	hello.Set("hello");

	writer.Write(hello);
	EventLoop::Get()->Reset(&writeTimeout);
}

int
main(int, char* argv[])
{
	IOProcessor::Get()->Init();
	
	if (!PaxosConfig::Get()->Init(argv[1]))
		ASSERT_FAIL();

	ReplicatedLog::Get()->Init();
	
	KeyspaceDB kdb;
	kdb.Init();

	HttpServer proto;
	proto.Init(&kdb);

	EventLoop::Get()->Run();

	return 0;
}
