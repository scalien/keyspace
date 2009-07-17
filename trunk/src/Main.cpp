#include <sys/types.h>
#include <pwd.h>

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
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceServer.h"
#include "Application/TimeCheck/TimeCheck.h"

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

	Log_Message(VERSION_FMT_STRING " started",
		VERSION_REVISION_LENGTH, VERSION_REVISION_NUMBER);

	if (!IOProcessor::Init(Config::GetIntValue("io.maxfd", 1024)))
		STOP_FAIL("Cannot initalize IOProcessor!", 1);

	// after io is initialized, drop root rights
	user = Config::GetValue("daemon.user", NULL);
	if (user != NULL && *user && (getuid() == 0 || geteuid() == 0)) 
	{
		struct passwd *pw = getpwnam(user);
		if (!pw)
			STOP_FAIL(rprintf("Cannot setuid to %s", user), 1);
		
		setuid(pw->pw_uid);
	}
	
	DatabaseConfig dbConfig;
	dbConfig.dir = Config::GetValue("database.dir", DATABASE_CONFIG_DIR);
	dbConfig.pageSize = Config::GetIntValue("database.pageSize", DATABASE_CONFIG_PAGE_SIZE);
	dbConfig.cacheSize = Config::GetIntValue("database.cacheSize", DATABASE_CONFIG_CACHE_SIZE);
	dbConfig.logBufferSize = Config::GetIntValue("database.logBufferSize", DATABASE_CONFIG_LOG_BUFFER_SIZE);
	dbConfig.checkpointTimeout = Config::GetIntValue("database.checkpointTimeout", DATABASE_CONFIG_CHECKPOINT_TIMEOUT);
	dbConfig.verbose = Config::GetBoolValue("database.verbose", DATABASE_CONFIG_VERBOSE);

	if (!database.Init(dbConfig))
		STOP_FAIL("Cannot initialize database!", 1);
	
	if (!RCONF->Init())
		STOP_FAIL("Cannot initialize paxos!", 1);

	KeyspaceDB* kdb;
	TimeCheck *tc;
	if (RCONF->GetNumNodes() > 1)
	{
		RLOG->Init(Config::GetBoolValue("paxos.useSoftClock", true));
		
		tc = new TimeCheck;
		if (Config::GetBoolValue("timecheck.active", true))
			tc->Init();
		
		kdb = new ReplicatedKeyspaceDB;
	}
	else
	{
		kdb = new SingleKeyspaceDB;
	}

	kdb->Init();
	
	HttpServer protoHttp;
	protoHttp.Init(kdb, Config::GetIntValue("http.port", 8080));

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
