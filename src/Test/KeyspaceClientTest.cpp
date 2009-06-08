#include "Application/Keyspace/Client/KeyspaceClient.h"
#include "System/Stopwatch.h"
#include "System/Config.h"
#include <signal.h>


void IgnorePipeSignal()
{
	sigset_t	sigset;
	
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGPIPE);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
}

// TODO
//
// SET, GET (1, 20, 50, 100 clients)
// key size: [10, 100]
// value size: [10 .. 100000]
// Dataset total 100MB (w/o keys)
//
//

class TestConfig
{
public:
	enum Type
	{
		GET,
		DIRTYGET,
		SET
	};
	int					type;
	const char*			typeString;
	int					numClients;
	int					keySize;
	int					valueSize;
	int					datasetTotal;
	DynArray<128>		key;
	DynArray<1024>		value;
	DynArray<100>		padding;
	
	void SetType(const char* s)
	{
		typeString = s;
		if (strcmp(s, "get") == 0)
		{
			type = GET;
			return;
		}
		if (strcmp(s, "dirtyget") == 0)
		{
			type = DIRTYGET;
			return;
		}
		if (strcmp(s, "set") == 0)
		{
			type = SET;
			return;
		}
	}
};

int KeyspaceClientGetTest(Keyspace::Client& client, TestConfig& conf)
{
	int			status;
	int			numTest;
	Stopwatch	sw;

	conf.value.Reallocate(conf.valueSize, false);
	
	Log_SetTrace(false);
	sw.Reset();

	numTest = conf.datasetTotal / conf.valueSize;
	for (int i = 0; i < numTest; i++)
	{
		conf.key.Writef("key%B:%d", conf.padding.length, conf.padding.buffer, i);
		sw.Start();

		if (conf.type == TestConfig::GET)
			status = client.Get(conf.key, conf.value);
		else
			status = client.DirtyGet(conf.key, conf.value);
		
		sw.Stop();

		if (status != KEYSPACE_OK)
		{
			Log_SetTrace(true);
			Log_Message("Test failed (%s failed after %d)", i, conf.typeString);
			return 1;
		}
	}

	Log_SetTrace(true);
	Log_Message("Test succeeded, %s/sec = %lf", conf.typeString, numTest / (sw.elapsed / 1000.0));
	
	return 0;
}

int KeyspaceClientSetTest(Keyspace::Client& client, TestConfig& conf)
{
	int			status;
	int			numTest;
	Stopwatch	sw;

	// prepare value
	conf.value.Clear();
	for (int i = 0; i < conf.valueSize; i++)
	{
		char c = (char) (i % 10) + '0';
		conf.value.Append(&c, 1);
	}
	
	numTest = conf.datasetTotal / conf.valueSize;
	for (int i = 0; i < numTest; i++)
	{
		conf.key.Writef("key%B:%d", conf.padding.length, conf.padding.buffer, i);
		status = client.Set(conf.key, conf.value, false);
		if (status != KEYSPACE_OK)
		{
			Log_Message("Test failed (%s failed after %d)", i, conf.typeString);
			return 1;
		}
	}

	Log_SetTrace(false);
	sw.Reset();
	sw.Start();

	status = client.Submit();
	if (status != KEYSPACE_OK)
	{
		Log_Message("Test failed (Submit failed)");
		return 1;
	}
	
	sw.Stop();
	Log_Message("Test succeeded, set/sec = %lf", numTest / (sw.elapsed / 1000.0));
	
	return 0;
}

int KeyspaceClientTest2(int , char **argv)
{
	char				**nodes;
	int					nodec;
	int					status;
	Keyspace::Client	client;
	int64_t				timeout;
	TestConfig			testConf;

	Config::Init("client.conf");
	
	IgnorePipeSignal();
	Log_SetTrace(Config::GetBoolValue("log.trace", false));
	Log_SetTimestamping(true);

	testConf.SetType(argv[1]);
	testConf.numClients = atoi(argv[2]);
	testConf.keySize = atoi(argv[3]);
	testConf.valueSize = atoi(argv[4]);

	nodec = Config::GetListNum("paxos.endpoints");
	if (nodec <= 0)
	{
		Log_Message("Bad configuration");
		return 1;
	}

	nodes = new char*[nodec];
	for (int i = 0; i < nodec; i++)
		nodes[i] = (char*) Config::GetListValue("paxos.endpoints", i, NULL);
	
	timeout = Config::GetIntValue("paxos.timeout", 10000);
	testConf.datasetTotal = Config::GetIntValue("dataset.total", 100 * 1000000);
	
	status = client.Init(nodec, nodes, timeout);
	if (status < 0)
		return 1;

	
	for (int i = 0; i < testConf.keySize - 10; i++)
	{
		char c = (char) (i % 10) + '0';
		testConf.padding.Append(&c, 1);
	}
	
	Log_Message("Test type = %s, numClients = %d, keySize = %d, valueSize = %d",
			testConf.typeString, testConf.numClients, testConf.keySize, testConf.valueSize);
		
	if (testConf.type == TestConfig::SET)
		return KeyspaceClientSetTest(client, testConf);
	else
		return KeyspaceClientGetTest(client, testConf);
}


int KeyspaceClientTest()
{	
	char				*nodes[] = {"127.0.0.1:7080", "127.0.0.1:7081", "127.0.01:7082"};
//	char				*nodes[] = {"127.0.0.1:7080"};
	DynArray<128>		key;
	DynArray<1024>		value;
	DynArray<36>		reference;
	int64_t				num;
	int					status;
	Keyspace::Client	client;
	Keyspace::Result*	result;
	Stopwatch			sw;
	
	IgnorePipeSignal();

	Log_SetTrace(true);
	Log_SetTimestamping(true);
	
	status = client.Init(SIZE(nodes), nodes, 3000);
	if (status < 0)
		return 1;

	reference.Writef("1234567890");

	key.Writef("%s", "test:0");
	value.Set(reference);
	status = client.Set(key, value);
	if (status != KEYSPACE_OK)
	{
		Log_Trace("SET failed");
		return 1;
	}
	
	value.Clear();
	status = client.Get(key, value);
	if (status != KEYSPACE_OK)
	{
		Log_Trace("GET failed");
		return 1;
	}
	if (!(value == reference))
	{
		Log_Trace("GET failed, value != reference");
		return 1;
	}

	// LIST test
	status = client.List(key);
	if (status != KEYSPACE_OK)
	{
		Log_Trace("LIST failed");
		return 1;
	}

	result = client.GetResult(status);
	while (result)
	{
		Log_Trace("%.*s", result->Key().length, result->Key().buffer);
		result = result->Next(status);		
	}
	
	// ADD test
	status = client.Add(key, 1, num);
	if (status != KEYSPACE_OK || num != 1234567891UL)
	{
		Log_Trace("ADD failed");
		return 1;
	}
	
	// DELETE test
	status = client.Delete(key);
	if (status != KEYSPACE_OK)
	{
		Log_Trace("DELETE failed");
		return 1;
	}
	
	// TESTANDSET test
	value.Writef("%U", num);
	status = client.TestAndSet(key, reference, value);
	if (status == KEYSPACE_OK)
	{
		Log_Trace("TESTANDSET failed");
		return 1;
	}
	
	// SET test (again)
	status = client.Set(key, reference);
	if (status != KEYSPACE_OK)
	{
		Log_Trace("SET failed");
		return 1;
	}

	status = client.TestAndSet(key, reference, value);
	if (status != KEYSPACE_OK)
	{
		Log_Trace("TESTANDSET failed");
		return 1;
	}
	
	// LISTP test
	status = client.ListP(key);
	if (status != KEYSPACE_OK)
	{
		Log_Trace("LISTP failed");
		return 1;
	}

	result = client.GetResult(status);
	while (result)
	{
		Log_Trace("%.*s: %.*s", result->Key().length, result->Key().buffer, result->Value().length, result->Value().buffer);
		result = result->Next(status);		
	}

	status = client.Begin();
	if (status != KEYSPACE_OK)
	{
		Log_Trace("Begin() failed");
		return 1;
	}

	num = 100000;
	value.Writef("0123456789012345678901234567890123456789");
	for (int i = 0; i < num; i++)
	{
		key.Writef("test_:%d", i);
		status = client.Set(key, value, false);
		if (status != KEYSPACE_OK)
		{
			Log_Trace("Set(submit=false) failed");
			return 1;
		}
	}
	
	Log_SetTrace(false);
	sw.Reset();
	sw.Start();

	status = client.Submit();
	if (status != KEYSPACE_OK)
	{
		Log_Trace("Submit() failed");
		return 1;
	}
	
	sw.Stop();
	Log_SetTrace(true);
	Log_Trace("set/sec = %lf", num / (sw.elapsed / 1000.0));
	
	// random GET test
	client.DistributeDirty(true);
	for (int i = 0; i < 100; i++)
	{
		key.Writef("test:%d", i);
		status = client.DirtyGet(key);
		if (status != KEYSPACE_OK)
		{
			Log_Trace("Submit() failed");
			return 1;
		}		
	}
	
//	key.Writef("");
//	status = client.List(key);
//	result = client.GetResult(status);
//	while (result)
//	{
//		Log_Trace("%.*s", result->Key().length, result->Key().buffer);
//		result = result->Next(status);
//	}

//	key.Writef("");
//	status = client.ListP(key);
//	result = client.GetResult(status);
//	while (result)
//	{
//		Log_Trace("%.*s: %.*s", result->Key().length, result->Key().buffer, result->Value().length, result->Value().buffer);
//		result = result->Next(status);
//	}
	

//	sw.Start();
//	num = 100000;
//	for (int i = 0; i < num; i++)
//	{
//		key.Printf("user:%d", i);
//		value.Clear();
//		client.DirtyGet(key, value);
//	}
//	sw.Stop();
//	Log_SetTrace(true);
//	Log_Trace("read/sec = %lf", num / (sw.elapsed / 1000.0));
	
	return 0;

	// get a key named "counter"
	key.Writef("counter");
	client.Get(key);
	result = client.GetResult(status);
	if (status == KEYSPACE_OK && result)
	{
		Log_Trace("Get result: key = %.*s, value = %.*s", result->Key().length, result->Key().buffer,
				  result->Value().length, result->Value().buffer);
		result->Close();
	}

	key.Writef("user:");		

	// list all keys starting with "user:"
	client.List(key);
	result = client.GetResult(status);
	while (status == KEYSPACE_OK && result)
	{
		Log_Trace("List result: key = %.*s", result->Key().length, result->Key().buffer);
		result = result->Next(status);
	}
	
	// list all keys and values starting with "user:" (List Key-Value Pairs)
	client.ListP(key);
	result = client.GetResult(status);
	while (status == KEYSPACE_OK && result)
	{
		Log_Trace("ListP result: key = %.*s, value = %.*s", result->Key().length, result->Key().buffer,
				  result->Value().length, result->Value().buffer);		
		result = result->Next(status);
	}
	
	return 0;
}

#ifndef TEST

int
main(int argc, char** argv)
{
	//KeyspaceClientTest();
	KeyspaceClientTest2(argc, argv);

	return 0;
}

#endif
