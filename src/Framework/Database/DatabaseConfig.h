#ifndef DATABASE_CONFIG_H
#define DATABASE_CONFIG_H

#define DATABASE_CONFIG_DIR					"."
#define DATABASE_CONFIG_PAGE_SIZE			4096
#define DATABASE_CONFIG_CACHE_SIZE			100*MB
#define DATABASE_CONFIG_LOG_BUFFER_SIZE		2*MB
#define DATABASE_CONFIG_LOG_MAX_FILE		0
#define DATABASE_CONFIG_CHECKPOINT_TIMEOUT	60000
#define DATABASE_CONFIG_VERBOSE				false

class DatabaseConfig
{
public:
	DatabaseConfig()
	{
		dir = DATABASE_CONFIG_DIR;
		pageSize = DATABASE_CONFIG_PAGE_SIZE;
		cacheSize = DATABASE_CONFIG_CACHE_SIZE;
		logBufferSize = DATABASE_CONFIG_LOG_BUFFER_SIZE;
		logMaxFile = DATABASE_CONFIG_LOG_MAX_FILE;
		checkpointTimeout = DATABASE_CONFIG_CHECKPOINT_TIMEOUT;
		verbose = DATABASE_CONFIG_VERBOSE;
	}
	
	const char*	dir;
	int			pageSize;
	int			cacheSize;
	int			logBufferSize;
	int			logMaxFile;
	int			checkpointTimeout;
	bool		verbose;
};

#endif
