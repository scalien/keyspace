#include <sys/types.h>

#include "Version.h"
#include "System/Config.h"
#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Framework/Database/Database.h"
#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Application/Keyspace/Database/SingleKeyspaceDB.h"
#include "Application/Keyspace/Database/ReplicatedKeyspaceDB.h"
#include "Application/Keyspace/Protocol/HTTP/HttpServer.h"
#include "Application/Keyspace/Protocol/HTTP/HttpApiHandler.h"
#include "Application/Keyspace/Protocol/HTTP/HttpFileHandler.h"
#include "Application/Keyspace/Protocol/HTTP/HttpKeyspaceHandler.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceServer.h"

#ifdef DEBUG
#define VERSION_FMT_STRING "Keyspace v" VERSION_STRING " r%.*s (DEBUG build date " __DATE__ " " __TIME__ ")"
#else
#define VERSION_FMT_STRING "Keyspace v" VERSION_STRING " r%.*s"
#endif


int main(int argc, char* argv[])
{
	int			logTargets;
	const char*	user;
	
	if (argc == 1)
	{
		fprintf(stderr, "You did not specify a config file!\n");
		fprintf(stderr, "Starting in single mode with defaults...\n");
		fprintf(stderr, "Using database.dir = '%s'\n", DATABASE_CONFIG_DIR);
	}
	else if (argc == 2)	
	{
		if (!Config::Init(argv[1]))
			STOP_FAIL("Cannot open config file", 1);
	}
	else
	{
		fprintf(stderr, "usage: %s <config-file>\n", argv[0]);
		STOP_FAIL("invalid arguments", 1);
	}
	 	
	logTargets = 0;
	if (Config::GetListNum("log.targets") == 0)
		logTargets = LOG_TARGET_STDOUT;
	for (int i = 0; i < Config::GetListNum("log.targets"); i++)
	{
		if (strcmp(Config::GetListValue("log.targets", i, ""), "file") == 0)
		{
			logTargets |= LOG_TARGET_FILE;
			Log_SetOutputFile(Config::GetValue("log.file", NULL), 
							Config::GetBoolValue("log.truncate", false));
		}
		if (strcmp(Config::GetListValue("log.targets", i, NULL), "stdout") == 0)
			logTargets |= LOG_TARGET_STDOUT;
		if (strcmp(Config::GetListValue("log.targets", i, NULL), "stderr") == 0)
			logTargets |= LOG_TARGET_STDERR;
	}
	Log_SetTarget(logTargets);
	Log_SetTrace(Config::GetBoolValue("log.trace", false));
	//Log_SetTrace(Config::GetBoolValue("log.trace", true));
	Log_SetTimestamping(Config::GetBoolValue("log.timestamping", false));

	Log_Message(VERSION_FMT_STRING " started",
		VERSION_REVISION_LENGTH, VERSION_REVISION_NUMBER);

	if (!IOProcessor::Init(Config::GetIntValue("io.maxfd", 1024)))
		STOP_FAIL("Cannot initalize IOProcessor!", 1);

	// after io is initialized, drop root rights
	user = Config::GetValue("daemon.user", NULL);
	if (!ChangeUser(user))
		STOP_FAIL(rprintf("Cannot setuid to %s", user), 1);
	
	DatabaseConfig dbConfig;
	dbConfig.dir = Config::GetValue("database.dir", DATABASE_CONFIG_DIR);
	dbConfig.pageSize = Config::GetIntValue("database.pageSize", DATABASE_CONFIG_PAGE_SIZE);
	dbConfig.cacheSize = Config::GetIntValue("database.cacheSize", DATABASE_CONFIG_CACHE_SIZE);
	dbConfig.logBufferSize = Config::GetIntValue("database.logBufferSize", DATABASE_CONFIG_LOG_BUFFER_SIZE);
	dbConfig.checkpointTimeout = Config::GetIntValue("database.checkpointTimeout", DATABASE_CONFIG_CHECKPOINT_TIMEOUT);
	dbConfig.verbose = Config::GetBoolValue("database.verbose", DATABASE_CONFIG_VERBOSE);

	if (!database.Init(dbConfig))
		STOP_FAIL("Cannot initialize database!", 1);
	
 	dbWriter.Init(1);
	dbReader.Init(Config::GetIntValue("database.numReaders", 20));
	
	if (!RCONF->Init())
		STOP_FAIL("Cannot initialize paxos!", 1);

	KeyspaceDB* kdb;
	if (RCONF->GetNumNodes() > 1)
	{
		RLOG->Init(Config::GetBoolValue("paxos.useSoftClock", true));
		kdb = new ReplicatedKeyspaceDB;
	}
	else
	{
		kdb = new SingleKeyspaceDB;
	}

	kdb->Init();
	
	HttpServer protoHttp;
	int httpPort = Config::GetIntValue("http.port", 8080);
	if (httpPort)
	{
		protoHttp.Init(httpPort);
	
		HttpApiHandler httpApiHandler(kdb);
		protoHttp.RegisterHandler(&httpApiHandler);

		HttpKeyspaceHandler httpKeyspaceHandler(kdb);
		protoHttp.RegisterHandler(&httpKeyspaceHandler);

		HttpFileHandler httpFileHandler(
							Config::GetValue("http.documentRoot", "admin"),
							Config::GetValue("http.prefix", "/"));
		protoHttp.RegisterHandler(&httpFileHandler);	
	}

	KeyspaceServer protoKeyspace;
	protoKeyspace.Init(kdb, Config::GetIntValue("keyspace.port", 7080));

	EventLoop::Run();
	
	Log_Message("Keyspace shutting down.");
	kdb->Shutdown();
	RLOG->Shutdown();
	dbReader.Shutdown();
	dbWriter.Shutdown();
	database.Shutdown();
}
