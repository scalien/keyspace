#include "System/Config.h"
#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Application/Keyspace/Database/SingleKeyspaceDB.h"
#include "Application/Keyspace/Database/ReplicatedKeyspaceDB.h"
#include "Application/Keyspace/Protocol/HTTP/HttpServer.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceServer.h"
#include "Application/TimeCheck/TimeCheck.h"

int main(int argc, char* argv[])
{
	int		logTargets;

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s <config-file>\n", argv[0]);
		return 1;
	}
	
	if (!Config::Init(argv[1]))
		STOP_FAIL("Cannot open config file", 1);

	logTargets = 0;
	for (int i = 0; i < Config::GetListNum("log.targets"); i++)
	{
		if (strcmp(Config::GetListValue("log.targets", i, ""), "file") == 0)
		{
			logTargets |= LOG_TARGET_FILE;
			Log_SetOutputFile(Config::GetValue("log.file", NULL));
		}
		if (strcmp(Config::GetListValue("log.targets", i, NULL), "stdout") == 0)
			logTargets |= LOG_TARGET_STDOUT;
		if (strcmp(Config::GetListValue("log.targets", i, NULL), "stderr") == 0)
			logTargets |= LOG_TARGET_STDERR;

	}
	Log_SetTarget(logTargets);
	Log_SetTrace(Config::GetBoolValue("log.trace", false));
	Log_SetTimestamping(Config::GetBoolValue("log.timestamping", false));

	if (!IOProcessor::Init(Config::GetIntValue("io.maxfd", 1024)))
		STOP_FAIL("Cannot initalize IOProcessor!", 1);
	
	if (!ReplicatedConfig::Get()->Init())
		STOP_FAIL("Cannot initialize paxos!", 1);

	TimeCheck tc;
	tc.Init();
	
	EventLoop::Run();
}
