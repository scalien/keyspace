#include "Version.h"
#include "System/Config.h"
#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Protocol/HTTP/HttpServer.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceServer.h"

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "usage: %s <config-file>\n", argv[0]);
		return 1;
	}
	
	if (!Config::Init(argv[1]))
		ASSERT_FAIL();

	Log_SetTimestamping(Config::GetBoolValue("log.timestamping", false));
	Log_Message("Keyspace v" VERSION_STRING " r%.*s started", VERSION_REVISION_LENGTH, VERSION_REVISION_NUMBER);

	IOProcessor::Init(Config::GetIntValue("io.maxfd", 1024));
	database.Init(Config::GetValue("database.dir", "."));	
	
	if (!PaxosConfig::Get()->Init())
		ASSERT_FAIL();

	if (PaxosConfig::Get()->numNodes > 1)
		ReplicatedLog::Get()->Init();
	
	KeyspaceDB kdb;
	kdb.Init();
	
	HttpServer protoHttp;
	protoHttp.Init(&kdb, Config::GetIntValue("http.port", 8080));

	KeyspaceServer protoKeyspace;
	protoKeyspace.Init(&kdb, Config::GetIntValue("keyspace.port", 7080));

	EventLoop::Run();
}
