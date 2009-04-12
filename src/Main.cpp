#include "System/Config.h"
#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Protocol/HttpServer.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceServer.h"

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("usage: %s <config-file>\n", argv[0]);
		return 1;
	}
	
	//Log_SetTimestamping(true);

	if (!Config::Init(argv[1]))
		ASSERT_FAIL();

	IOProcessor::Init();
	database.Init(Config::GetValue("database.dir", "."));	
	
	if (!PaxosConfig::Get()->Init())
		ASSERT_FAIL();

	ReplicatedLog::Get()->Init();
	
	KeyspaceDB kdb;
	kdb.Init();
	
	HttpServer protoHttp;
	protoHttp.Init(&kdb, Config::GetIntValue("http.port", 8080));

	KeyspaceServer protoKeyspace;
	protoKeyspace.Init(&kdb, Config::GetIntValue("keyspace.port", 7080));

	EventLoop::Run();
}
