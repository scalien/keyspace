#include "Framework/Database/Database.h"
#include "Framework/Database/Table.h"
#include "Framework/Database/Transaction.h"
#include "Application/Keyspace/Client/KeyspaceClient.h"
#include "System/Config.h"
#include "TestConfig.h"

void GenRandomString(ByteString& bs, size_t length);


int DatabaseSetup()
{
	DatabaseConfig dbConfig;
	dbConfig.dir = Config::GetValue("database.dir", DATABASE_CONFIG_DIR);
	dbConfig.pageSize = Config::GetIntValue("database.pageSize", DATABASE_CONFIG_PAGE_SIZE);
	dbConfig.cacheSize = Config::GetIntValue("database.cacheSize", DATABASE_CONFIG_CACHE_SIZE);
	dbConfig.logBufferSize = Config::GetIntValue("database.logBufferSize", DATABASE_CONFIG_LOG_BUFFER_SIZE);
	dbConfig.checkpointTimeout = Config::GetIntValue("database.checkpointTimeout", DATABASE_CONFIG_CHECKPOINT_TIMEOUT);
	dbConfig.verbose = Config::GetBoolValue("database.verbose", DATABASE_CONFIG_VERBOSE);

	if (!database.Init(dbConfig))
		return 1;

	return 0;
}

int DatabaseSetTest(TestConfig& conf)
{
	int			numTest;
	Stopwatch	sw;
	Table*		table;
	Transaction* tx;
	bool		ret;
	int			limit = 16*KB;
	int			sum;

	if (conf.argc < 5)
	{
		Log_Message("\n\tusage: %s <keySize> <valueSize>", conf.typeString);
		return 1;
	}
	
	Log_SetTrace(true);
	
	if (DatabaseSetup())
	{
		Log_Message("Cannot initialize database!", 1);
		return 1;
	}
	
	table = database.GetTable("keyspace");
	if (!table)
	{
		Log_Message("Cannot initialize table!", 1);
		return 1;
	}
	
	conf.SetKeySize(atoi(conf.argv[3]));
	conf.SetValueSize(atoi(conf.argv[4]));

	Log_Message("Test type = %s, keySize = %d, valueSize = %d",
			conf.typeString, conf.keySize, conf.valueSize);
	
	tx = NULL;
	tx = new Transaction(table);
	sw.Start();
	tx->Begin();
	sw.Stop();
	
	sum = 0;
	numTest = conf.datasetTotal / conf.valueSize;
	for (int i = 0; i < numTest; i++)
	{
		if (conf.rndkey)
			GenRandomString(conf.key, conf.keySize);
		else
			conf.key.Writef("key%B:%d", conf.padding.length, conf.padding.buffer, i);
		
		sw.Start();
		ret = table->Set(tx, conf.key, conf.value);
		sw.Stop();
		if (!ret)
		{
			Log_Message("Test failed, ret = %s (%s failed after %d)", ret ? "true" : "false", conf.typeString, i);
			return 1;
		}
		
		sum += conf.keySize + conf.valueSize;
		if (sum > limit)
		{			
			sw.Start();
			tx->Commit();
			sw.Stop();
			
			double mbps = sum / (sw.elapsed / 1000.0) / 1000000;
			Log_Message("num = %d, elapsed = %ld, thruput = %lf MB/s", i, sw.elapsed, mbps);
			sw.Reset();
			
			sw.Start();
			tx->Begin();
			sw.Stop();

			sum = 0;
		}
	}

	sw.Start();
	tx->Commit();
	sw.Stop();

	double mbps = (conf.valueSize + conf.keySize) * numTest / (sw.elapsed / 1000.0) / 1000000;
	Log_Message("Test succeeded, %s/sec = %lf (num = %d, elapsed = %ld, thruput = %lf MB/s)", conf.typeString, numTest / (sw.elapsed / 1000.0), numTest, sw.elapsed, mbps);
	
	return 0;
}
