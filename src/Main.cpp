#include "Version.h"
#include "System/Config.h"
#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Application/Keyspace/Database/SingleKeyspaceDB.h"
#include "Application/Keyspace/Database/ReplicatedKeyspaceDB.h"
#include "Application/Keyspace/Protocol/HTTP/HttpServer.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceServer.h"
#include "Application/TimeCheck/TimeCheck.h"
#include "Application/Console/Console.h"

#ifdef DEBUG
#define VERSION_FMT_STRING "Keyspace v" VERSION_STRING " r%.*s started (DEBUG build date " __DATE__ " " __TIME__ ")"
#else
#define VERSION_FMT_STRING "Keyspace v" VERSION_STRING " r%.*s"
#endif

#define MATCHCMD(cmd, cstr) (strncmp(cmd, "" cstr, sizeof(cstr) - 1) == 0)

class DebugCommand : public ConsoleCommand
{
public:
	virtual void	Execute(const char* cmd, const char *args, ConsoleConn* conn)
	{
		if (MATCHCMD(cmd, "crash"))
		{
			char *p = NULL;
			p[0] = 0;
		}
		else if (MATCHCMD(cmd, "exit"))
		{
			int code = 0;
			
			if (args)
				code = atoi(args);
			_exit(code);
		}
		else if (MATCHCMD(cmd, "log-reload"))
		{
			const char LOG_RELOAD_MSG[] = "Log file reloaded\n";
			Log_SetOutputFile(Config::GetValue("log.file", NULL));
			conn->Write(LOG_RELOAD_MSG, sizeof(LOG_RELOAD_MSG));
			Log_Message(LOG_RELOAD_MSG);
		}
		else if (MATCHCMD(cmd, "trace"))
		{
			if (args && atoi(args))
				Log_SetTrace(true);
			else
				Log_SetTrace(false);
		}
		else if (MATCHCMD(cmd, "version"))
		{
			char version[128];
			int ret;
			ret = snprintf(version, sizeof(version), VERSION_FMT_STRING "\n", 
				(int) VERSION_REVISION_LENGTH, VERSION_REVISION_NUMBER);
			if (ret > (int) sizeof(version))
				ret = sizeof(version);
			conn->Write(version, ret);
		}
		else if (MATCHCMD(cmd, "quit"))
		{
			conn->Disconnect();
		}
	}
};

int main(int argc, char* argv[])
{
	int		logTargets;
	
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
	if (getuid() == 0 || geteuid() == 0) 
	{
		struct passwd *pw = getpwnam("daemon");
		if (!pw)
			STOP_FAIL("Cannot setuid to daemon", 1);
		
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
	
	if (!ReplicatedConfig::Get()->Init())
		STOP_FAIL("Cannot initialize paxos!", 1);

	KeyspaceDB* kdb;
	TimeCheck *tc;
	if (ReplicatedConfig::Get()->numNodes > 1)
	{
		ReplicatedLog::Get()->Init(Config::GetBoolValue("paxos.useSoftClock", true));
		
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

	Console console;
	if (Config::GetIntValue("console.port", 22222) != 0)
		console.Init(Config::GetIntValue("console.port", 22222), 
					 Config::GetValue("console.interface", "127.0.0.1"));

	DebugCommand debug;
	console.RegisterCommand(&debug);

	EventLoop::Run();
}
