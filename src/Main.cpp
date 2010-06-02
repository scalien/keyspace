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
#include "Application/HTTP/HttpServer.h"
#include "Application/HTTP/HttpFileHandler.h"
#include "Application/Keyspace/Protocol/HTTP/HttpApiHandler.h"
#include "Application/Keyspace/Protocol/HTTP/HttpKeyspaceHandler.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceServer.h"

#ifdef DEBUG
#define VERSION_FMT_STRING "Keyspace v" VERSION_STRING " (DEBUG build date " __DATE__ " " __TIME__ ")"
#else
#define VERSION_FMT_STRING "Keyspace v" VERSION_STRING 
#endif

int main(int argc, char* argv[])
{
	enum		{ single, replicated, missing } mode;
	int			logTargets;
	const char*	user;
	char		buf[4096];
	bool		deleteDB;
	bool		firstRun;

	firstRun = true;
	
	mode = missing;
	if (argc == 1)
	{
		fprintf(stderr, "You did not specify a config file!\n");
		fprintf(stderr, "Starting in single mode with defaults...\n");
		fprintf(stderr, "Using database.dir = '%s'\n", DATABASE_CONFIG_DIR);
		mode = single;
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
	
	if (strcmp("single", Config::GetValue("mode", "")) == 0)
		mode = single;
	else if (strcmp("replicated", Config::GetValue("mode", "")) == 0)
		mode = replicated;
	else if (mode == missing)
	{
		fprintf(stderr, "specify mode = single or mode = replicated\n");
		STOP_FAIL("invalid configuration file", 1);
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
	Log_SetTimestamping(Config::GetBoolValue("log.timestamping", false));

	Log_Message(VERSION_FMT_STRING " started");

	run:
	{
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
		dbConfig.directDB = Config::GetBoolValue("database.directDB", DATABASE_CONFIG_DIRECT_DB);
		dbConfig.txnNoSync = Config::GetBoolValue("database.txnNoSync", DATABASE_CONFIG_TXN_NOSYNC);
		dbConfig.txnWriteNoSync = Config::GetBoolValue("database.txnWriteNoSync", DATABASE_CONFIG_TXN_WRITE_NOSYNC);

		if (Config::GetBoolValue("database.warmCache", true) && firstRun)
			WarmCache((char*)dbConfig.dir, dbConfig.cacheSize);

		if (firstRun)
			Log_Message("Opening database...");
		if (!database.Init(dbConfig))
			STOP_FAIL("Cannot initialize database!", 1);
		if (firstRun)
			Log_Message("Database opened");

		dbWriter.Init(1);
		dbReader.Init(Config::GetIntValue("database.numReaders", 20));
		
		if (!RCONF->Init())
			STOP_FAIL("Cannot initialize paxos!", 1);

		KeyspaceDB* kdb;
		if (mode == replicated)
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
		HttpKeyspaceHandler httpKeyspaceHandler(kdb);
		
		int httpPort = Config::GetIntValue("http.port", 8080);
		if (httpPort)
		{
			protoHttp.Init(httpPort);
			protoHttp.RegisterHandler(&httpKeyspaceHandler);
		}

		KeyspaceServer protoKeyspace;
		protoKeyspace.Init(kdb, Config::GetIntValue("keyspace.port", 7080));
		
		EventLoop::Init();
		EventLoop::Run();
		EventLoop::Shutdown();
			
		if (mode == replicated)
			deleteDB = ((ReplicatedKeyspaceDB*)kdb)->DeleteDB();
		else
			deleteDB = false;
		
		protoKeyspace.Shutdown();
		protoHttp.Shutdown();
		
		kdb->Shutdown();
		delete kdb;
		dbReader.Shutdown();
		dbWriter.Shutdown();
		RLOG->Shutdown();
		database.Shutdown();
		IOProcessor::Shutdown();
		
		if (mode == replicated && deleteDB)
		{
//			snprintf(buf, SIZE(buf), "%s/__*", dbConfig.dir);
//			DeleteWC(buf);
			snprintf(buf, SIZE(buf), "%s/log*", dbConfig.dir);
			DeleteWC(buf);
			snprintf(buf, SIZE(buf), "%s/keyspace", dbConfig.dir);
			DeleteWC(buf);
#ifdef _WIN32
			MSleep(3000); // otherwise Windows won't let use reuse the same ports
#endif
			firstRun = false;
			goto run;
		}
	}

	Log_Message("Keyspace shutting down.");	
	Config::Shutdown();
	Log_Shutdown();

}
