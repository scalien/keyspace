#include "Version.h"
#include "System/Config.h"
#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Protocol/HTTP/HttpServer.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceServer.h"
#include "Application/TimeCheck/TimeCheck.h"

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "usage: %s <config-file>\n", argv[0]);
		return 1;
	}
	
	if (!Config::Init(argv[1]))
		STOP_FAIL("Cannot open config file", 1);

	Log_SetTrace(Config::GetBoolValue("log.trace", false));
	Log_SetTimestamping(Config::GetBoolValue("log.timestamping", false));
	Log_Message("Keyspace v" VERSION_STRING " r%.*s started",
		VERSION_REVISION_LENGTH, VERSION_REVISION_NUMBER);

	if (!IOProcessor::Init(Config::GetIntValue("io.maxfd", 1024)))
		STOP_FAIL("Cannot initalize IOProcessor!", 1);
		
	if (!database.Init(Config::GetValue("database.dir", "."),
					Config::GetIntValue("database.pageSize", 0),
					Config::GetIntValue("database.cacheSize", 0)))
		STOP_FAIL("Cannot initialize database!", 1);
	
	if (!PaxosConfig::Get()->Init())
		STOP_FAIL("Cannot initialize paxos!", 1);

	if (PaxosConfig::Get()->numNodes > 1)
		ReplicatedLog::Get()->Init();
	
	TimeCheck timeCheck;
	if (Config::GetBoolValue("timecheck.active", false))
		timeCheck.Init();
	
	KeyspaceDB kdb;
	kdb.Init();
	
	HttpServer protoHttp;
	protoHttp.Init(&kdb, Config::GetIntValue("http.port", 8080));

	KeyspaceServer protoKeyspace;
	protoKeyspace.Init(&kdb, Config::GetIntValue("keyspace.port", 7080));

	EventLoop::Run();
}
