#include "Application/Keyspace/Client/KeyspaceClient.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"
#include "System/Stopwatch.h"
#include "System/Config.h"
#include <signal.h>
#include <float.h>

using namespace Keyspace;
int KeyspaceClientTestSuite(Keyspace::Client& client);

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
		SET,
		LIST,
		LISTP,
		GETLATENCY,
		DIRTYGETLATENCY,
		SETLATENCY
	};
	
	int					type;
	const char*			typeString;
	int					keySize;
	int					valueSize;
	int					datasetTotal;
	DynArray<128>		key;
	DynArray<1024>		value;
	DynArray<100>		padding;
	bool				rndkey;
	int					argc;
	char**				argv;
	
	TestConfig()
	{
		rndkey = false;
	}
		
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
		if (strcmp(s, "rndset") == 0)
		{
			type = SET;
			rndkey = true;
			return;
		}
		if (strcmp(s, "list") == 0)
		{
			type = LIST;
			return;
		}
		if (strcmp(s, "listp") == 0)
		{
			type = LISTP;
			return;
		}
		if (strcmp(s, "getlatency") == 0)
		{
			type = GETLATENCY;
			return;
		}
		if (strcmp(s, "dirtygetlatency") == 0)
		{
			type = DIRTYGETLATENCY;
			return;
		}
		if (strcmp(s, "setlatency") == 0)
		{
			type = SETLATENCY;
			return;
		}
	}
};

static const char *Status(int status)
{
	switch (status)
	{
	case KEYSPACE_OK:
		return "OK";
	case KEYSPACE_FAILED:
		return "FAILED";
	case KEYSPACE_NOTMASTER:
		return "NOTMASTER";
	case KEYSPACE_ERROR:
		return "ERROR";
	default:
		return "<UNKNOWN>";
	}
}

int KeyspaceClientListTest(Keyspace::Client& client, TestConfig& conf)
{
	int				status;
	Stopwatch		sw;
	DynArray<1>		startKey;
	int				num;
	Result			*result;

	if (conf.argc < 5)
	{
		Log_Message("\n\tusage: %s <keySize> <valueSize>", conf.typeString);
		return -1;
	}

	conf.keySize = atoi(conf.argv[3]);
	conf.valueSize = atoi(conf.argv[4]);

	Log_Message("Test type = %s, keySize = %d, valueSize = %d",
			conf.typeString, conf.keySize, conf.valueSize);

	conf.key.Writef("key%B", conf.padding.length, conf.padding.buffer);

	sw.Reset();	
	sw.Start();

	if (conf.type == TestConfig::LIST)
		status = client.ListKeys(conf.key, startKey);
	else
		status = client.ListKeyValues(conf.key, startKey);

	sw.Stop();
	
	result = client.GetResult(status);
	if (status != KEYSPACE_OK || !result)
	{
		Log_Message("Test failed, status = %s (%s failed after %lu msec)", Status(status), conf.typeString, (unsigned long) sw.elapsed);
		return 1;
	}
	
	num = 0;
	while (result)
	{
		num++;
		result = result->Next(status);
	}
	
	Log_Message("Test succeeded (%s %d, elapsed %lu)", conf.typeString, num, (unsigned long) sw.elapsed);
	return 0;
}

int KeyspaceClientGetTest(Keyspace::Client& client, TestConfig& conf)
{
	int			status;
	int			numTest;
	Stopwatch	sw;

	if (conf.argc < 5)
	{
		Log_Message("\n\tusage: %s <keySize> <valueSize>", conf.typeString);
		return -1;
	}

	conf.keySize = atoi(conf.argv[3]);
	conf.valueSize = atoi(conf.argv[4]);

	Log_Message("Test type = %s, keySize = %d, valueSize = %d",
			conf.typeString, conf.keySize, conf.valueSize);

	client.DistributeDirty(true);
	conf.value.Reallocate(conf.valueSize, false);
	
	sw.Reset();

	client.Begin();

	numTest = conf.datasetTotal / conf.valueSize;
	for (int i = 0; i < numTest; i++)
	{
		conf.key.Writef("key%B:%d", conf.padding.length, conf.padding.buffer, i);
		//sw.Start();

		if (conf.type == TestConfig::GET)
			status = client.Get(conf.key, false, false);
		else
			status = client.DirtyGet(conf.key, false);
		
		//sw.Stop();

		if (status != KEYSPACE_OK)
		{
			Log_Message("Test failed, status = %s (%s failed after %d)", Status(status), conf.typeString, i);
			return 1;
		}
	}

	sw.Start();
	status = client.Submit();
	sw.Stop();

	if (status != KEYSPACE_OK)
	{
		Log_Message("Test failed, status = %s (%s)", Status(status), conf.typeString);
		return 1;
	}

	Log_Message("Test succeeded, %s/sec = %lf (num = %d, elapsed = %ld)", conf.typeString, numTest / (sw.elapsed / 1000.0), numTest, sw.elapsed);
	
	return 0;
}

void GenRandomString(ByteString& bs, size_t length)
{
	const char set[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	const size_t setsize = sizeof(set) - 1;
	unsigned int i;
	uint64_t seed = Now();
	uint64_t d = seed;

	assert(bs.size >= length);

	for (i = 0; i < length; i++) {
			// more about why these numbers were chosen:
			// http://en.wikipedia.org/wiki/Linear_congruential_generator
			d = (d * 1103515245UL + 12345UL) >> 16;
			bs.buffer[i] = set[d % setsize];
	}

	bs.length = length;
}

int KeyspaceClientSetTest(Keyspace::Client& client, TestConfig& conf)
{
	int			status;
	int			numTest;
	Stopwatch	sw;

	if (conf.argc < 5)
	{
		Log_Message("\n\tusage: %s <keySize> <valueSize>", conf.typeString);
		return -1;
	}
	
	conf.keySize = atoi(conf.argv[3]);
	conf.valueSize = atoi(conf.argv[4]);

	Log_Message("Test type = %s, keySize = %d, valueSize = %d",
			conf.typeString, conf.keySize, conf.valueSize);
	Log_Trace("Generating data...");

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
		if (conf.rndkey)
			GenRandomString(conf.key, conf.keySize);
		else
			conf.key.Writef("key%B:%d", conf.padding.length, conf.padding.buffer, i);
		status = client.Set(conf.key, conf.value, false);
		if (status != KEYSPACE_OK)
		{
			Log_Message("Test failed, status = %s (%s failed after %d)", Status(status), conf.typeString, i);
			return 1;
		}
	}

	Log_Message("Sending Submit()");

	sw.Reset();
	sw.Start();

	status = client.Submit();

	sw.Stop();
	if (status != KEYSPACE_OK)
	{
		Log_Message("Test failed, status = %s (Submit failed)", Status(status));
		return 1;
	}
	
	Log_Message("Test succeeded, %s/sec = %lf (num = %d, elapsed = %ld)", conf.typeString, numTest / (sw.elapsed / 1000.0), numTest, sw.elapsed);
	
	return 0;
}

int KeyspaceClientLatencyTest(Keyspace::Client& client, TestConfig& conf)
{
	int			status;
	int			numTest;
	Stopwatch	sw;
	double		minLatency = DBL_MAX;
	double		maxLatency = DBL_MIN;
	double		avgLatency = 0.0;
	double		latency;

	if (conf.argc < 5)
	{
		Log_Message("\n\tusage: %s <keySize> <valueSize>", conf.typeString);
		return -1;
	}
	
	conf.keySize = atoi(conf.argv[3]);
	conf.valueSize = atoi(conf.argv[4]);

	Log_Message("Test type = %s, keySize = %d, valueSize = %d",
			conf.typeString, conf.keySize, conf.valueSize);
	Log_Trace("Generating data...");

	// prepare value
	conf.value.Clear();
	for (int i = 0; i < conf.valueSize; i++)
	{
		char c = (char) (i % 10) + '0';
		conf.value.Append(&c, 1);
	}
	
	numTest = 1000;
	for (int i = 0; i < numTest; i++)
	{
		if (conf.rndkey)
			GenRandomString(conf.key, conf.keySize);
		else
			conf.key.Writef("key%B:%d", conf.padding.length, conf.padding.buffer, i);

		if (conf.type == TestConfig::GETLATENCY)
			status = client.Get(conf.key);
		else if (conf.type == TestConfig::DIRTYGETLATENCY)
			status = client.DirtyGet(conf.key);
		else if (conf.type == TestConfig::SETLATENCY)
			status = client.Set(conf.key, conf.value);
			
		if (status != KEYSPACE_OK)
		{
			Log_Message("Test failed, status = %s (%s failed after %d)", Status(status), conf.typeString, i);
			return 1;
		}
		
		latency = client.GetLatency();
		if (i > 0)
			avgLatency = (avgLatency * (i - 1)) / i + latency / i;
		else
			avgLatency = latency;
		if (latency < minLatency)
			minLatency = latency;
		if (latency > maxLatency)
			maxLatency = latency;
	}

	Log_Message("Test succeeded, %s: avg = %lf, min = %lf, max = %lf", conf.typeString, avgLatency, minLatency, maxLatency);
	
	return 0;
}


int KeyspaceClientTest(int argc, char **argv)
{
	const char			**nodes;
	int					nodec;
	int					status;
	Keyspace::Client	client;
	TestConfig			testConf;
	uint64_t			timeout;
	const char			*LOCAL_NODES[] = {"127.0.0.1:7080", "127.0.0.1:7081", "127.0.01:7082"};


	if (argc < 3)
	{
		Log_Message("usage:\n\t%s <configFile> <command>", argv[0]);
		Log_Message("\n\t\tcommand can be any of get, dirtyget, set, rndset, list, listp, getlatency, dirtygetlatency, setlatency, suite\n");
		return 1;
	}

	IgnorePipeSignal();
	Config::Init(argv[1]);
	Log_SetTrace(Config::GetBoolValue("log.trace", false));
	Log_SetTimestamping(true);

	nodec = Config::GetListNum("paxos.endpoints");
	if (nodec <= 0)
	{
		nodec = SIZE(LOCAL_NODES);
		nodes = LOCAL_NODES;
	}
	else
	{
		nodes = new const char*[nodec];
		for (int i = 0; i < nodec; i++)
			nodes[i] = Config::GetListValue("paxos.endpoints", i, NULL);
	}
	
	timeout = Config::GetIntValue("paxos.timeout", 10000);
	testConf.datasetTotal = Config::GetIntValue("dataset.total", 100 * 1000000);
	
	status = client.Init(nodec, nodes, timeout);
	if (status < 0)
		return 1;

	if (strcmp(argv[2], "suite") == 0)
	{
		return KeyspaceClientTestSuite(client);
	} 
	
	testConf.argc = argc;
	testConf.argv = argv;
	testConf.SetType(argv[2]);
	
	for (int i = 0; i < testConf.keySize - 10; i++)
	{
		char c = (char) (i % 10) + '0';
		testConf.padding.Append(&c, 1);
	}
	
	if (testConf.type == TestConfig::SET)
		return KeyspaceClientSetTest(client, testConf);
	if (testConf.type == TestConfig::LIST || testConf.type == TestConfig::LISTP)
		return KeyspaceClientListTest(client, testConf);
	if (testConf.type == TestConfig::GETLATENCY || testConf.type == TestConfig::DIRTYGETLATENCY || testConf.type == TestConfig::SETLATENCY)
		return KeyspaceClientLatencyTest(client, testConf);
	else
		return KeyspaceClientGetTest(client, testConf);
		
	return 1;
}


int KeyspaceClientTestSuite(Keyspace::Client& client)
{	
	DynArray<128>		key;
	DynArray<1024>		value;
	DynArray<36>		reference;
	DynArray<128>		newName;
	int64_t				num;
	int					status;
	Keyspace::Result*	result;
	Stopwatch			sw;
	const int			NUM_TEST_KEYS = 1000;
	
	reference.Writef("1234567890");

	//goto limitset;

	// basic SET test
	{
		key.Writef("%s", "test:0");
		value.Set(reference);
		status = client.Set(key, value);
		if (status != KEYSPACE_OK)
		{
			Log_Message("SET failed, status = %s", Status(status));
			return 1;
		}
		Log_Message("SET succeeded");
	}
	
	// basic GET test
	{
		value.Clear();
		status = client.Get(key, value);
		if (status != KEYSPACE_OK)
		{
			Log_Message("GET failed, status = %s", Status(status));
			return 1;
		}
		if (!(value == reference))
		{
			Log_Message("GET failed, value != reference");
			return 1;
		}
		Log_Message("GET succeeded");
	}


	// LISTKEYS test
	{
		DynArray<128>	startKey;
		
		status = client.ListKeys(key, startKey);
		if (status != KEYSPACE_OK)
		{
			Log_Message("LISTKEYS failed, status = %s", Status(status));
			return 1;
		}

		result = client.GetResult(status);
		while (result)
		{
			Log_Trace("LISTKEYS: %.*s", result->Key().length, result->Key().buffer);
			result = result->Next(status);		
		}
		
		Log_Message("LISTKEYS succeeded");
	}
	
	// ADD test
	{
		status = client.Add(key, 1, num);
		if (status != KEYSPACE_OK || num != 1234567891UL)
		{
			Log_Message("ADD failed, status = %s", Status(status));
			return 1;
		}

		Log_Message("ADD succeeded");
	}
	
	// DELETE test
	{
		status = client.Delete(key);
		if (status != KEYSPACE_OK)
		{
			Log_Message("DELETE failed, status = %s", Status(status));
			return 1;
		}
		
		Log_Message("DELETE succeeded");
	}
	
	// TESTANDSET test
	{
		value.Writef("%U", num);
		status = client.TestAndSet(key, reference, value);
		if (status == KEYSPACE_OK)
		{
			Log_Message("TESTANDSET failed, status = %s", Status(status));
			return 1;
		}

		Log_Message("TESTANDSET succeeded");
	}
	
	// SET test (again)
	{
		status = client.Set(key, reference);
		if (status != KEYSPACE_OK)
		{
			Log_Message("SET/TESTANDSET failed, status = %s", Status(status));
			return 1;
		}

		status = client.TestAndSet(key, reference, value);
		if (status != KEYSPACE_OK)
		{
			Log_Message("SET/TESTANDSET failed, status = %s", Status(status));
			return 1;
		}
		
		status = client.Set(key, reference);
		if (status != KEYSPACE_OK)
		{
			Log_Message("SET/TESTANDSET failed, status = %s", Status(status));
			return 1;
		}
		
		Log_Message("SET/TESTANDSET succeeded");
	}
	
	// RENAME test
	{
		newName.Writef("%s", "test:1");
		status = client.Rename(key, newName);
		if (status != KEYSPACE_OK)
		{
			Log_Message("RENAME failed, status = %s", Status(status));
			return 1;
		}
		
		Log_Message("RENAME succeeded");
	}
	
	// REMOVE test
	{
		status = client.Remove(newName);
		result = client.GetResult(status);
		if (status != KEYSPACE_OK || !result)
		{
			Log_Message("REMOVE failed, status = %s", Status(status));
			return 1;
		}
		if (!(result->Value() == reference))
		{
			Log_Message("REMOVE failed");
			return 1;
		}
		
		Log_Message("REMOVE succeeded");
	}
	
	// SET test (again)
	{
		status = client.Set(key, reference);
		if (status != KEYSPACE_OK)
		{
			Log_Message("SET/2 failed, status = %s", Status(status));
			return 1;
		}
		
		Log_Message("SET/2 succeeded");
	}
	

	// LISTKEYVALUES test
	{
		DynArray<128> startKey;
		status = client.ListKeyValues(key, startKey);
		if (status != KEYSPACE_OK)
		{
			Log_Message("LISTKEYVALUES failed, status = %s", Status(status));
			return 1;
		}

		result = client.GetResult(status);
		while (result)
		{
			Log_Trace("LISTKEYVALUES: %.*s, %.*s", result->Key().length, result->Key().buffer, result->Value().length, result->Value().buffer);
			result = result->Next(status);		
		}
		
		Log_Message("LISTKEYVALUES succeeded");
	}


	// batched SET test
	{
		status = client.Begin();
		if (status != KEYSPACE_OK)
		{
			Log_Message("SET/batched Begin() failed, status = %s", Status(status));
			return 1;
		}

		num = NUM_TEST_KEYS;
		value.Writef("0123456789012345678901234567890123456789");
		for (int i = 0; i < num; i++)
		{
			key.Writef("test:%d", i);
			status = client.Set(key, value, false);
			if (status != KEYSPACE_OK)
			{
				Log_Message("SET/batched failed after %d, status = %s", i, Status(status));
				return 1;
			}
		}
		
		bool trace = Log_SetTrace(false);
		sw.Reset();
		sw.Start();

		status = client.Submit();
		if (status != KEYSPACE_OK)
		{
			Log_Message("SET/batched Submit() failed, status = %s", Status(status));
			return 1;
		}
		
		sw.Stop();
		Log_SetTrace(trace);
		Log_Message("SET/batched succeeded, set/sec = %lf", num / (sw.elapsed / 1000.0));
	}
	
	// discrete GET test
	{
		sw.Reset();
		client.DistributeDirty(true);
		for (int i = 0; i < num; i++)
		{
			key.Writef("test:%d", i);
			sw.Start();
			status = client.DirtyGet(key);
			sw.Stop();
			if (status != KEYSPACE_OK && status != KEYSPACE_FAILED)
			{
				Log_Message("DIRTYGET/discrete failed, status = %s", Status(status));
				return 1;
			}		
		}
		
		Log_Message("DIRTYGET/discrete succeeded, get/sec = %lf", num / (sw.elapsed / 1000.0));
	}
	
	// batched GET test
	{
		sw.Reset();
		client.Begin();
		
		for (int i = 0; i < num; i++)
		{
			key.Writef("test:%d", i);
			status = client.Get(key, true, false);
			if (status != KEYSPACE_OK)
			{
				Log_Message("GET/batched failed, status = %s", Status(status));
				return 1;
			}		
		}
		
		sw.Start();
		status = client.Submit();
		sw.Stop();
		if (status != KEYSPACE_OK)
		{
			Log_Message("GET/batched failed, status = %s", Status(status));
			return 1;
		}
		
		Log_Message("GET/batched succeeded, get/sec = %lf", num / (sw.elapsed / 1000.0));		
	}

	// batched DIRTYGET test
	{
		client.DistributeDirty(true);
		sw.Reset();
		client.Begin();
		
		for (int i = 0; i < num; i++)
		{
			key.Writef("test:%d", i);
			status = client.DirtyGet(key, false);
			if (status != KEYSPACE_OK)
			{
				Log_Message("DIRTYGET/batched failed, status = %s", Status(status));
				return 1;
			}		
		}
		
		sw.Start();
		status = client.Submit();
		sw.Stop();
		if (status != KEYSPACE_OK)
		{
			Log_Message("DIRTYGET/batched failed, status = %s", Status(status));
			return 1;
		}
		
		Log_Message("DIRTYGET/batched succeeded, get/sec = %lf", num / (sw.elapsed / 1000.0));		
	}

	// batched DIRTYGET test #2 without distributing dirty reads between clients
	{
		client.DistributeDirty(false);
		sw.Reset();
		client.Begin();
		
		for (int i = 0; i < num; i++)
		{
			key.Writef("test:%d", i);
			status = client.DirtyGet(key, false);
			if (status != KEYSPACE_OK)
			{
				Log_Message("DIRTYGET/batched2 failed, status = %s", Status(status));
				return 1;
			}		
		}
		
		sw.Start();
		status = client.Submit();
		sw.Stop();
		if (status != KEYSPACE_OK)
		{
			Log_Message("DIRTYGET/batched2 failed, status = %s", Status(status));
			return 1;
		}
		
		Log_Message("DIRTYGET/batched2 succeeded, get/sec = %lf", num / (sw.elapsed / 1000.0));		
	}

	// paginated LISTKEYS
	{
		int i;
		DynArray<128> prefix;
		DynArray<128> startKey;

		//Log_SetTrace(true);

		num = 10;
		prefix.Writef("test:");
		status = client.ListKeys(prefix, startKey, num, false);
		result = client.GetResult(status);
		if (status != KEYSPACE_OK || !result)
		{
			Log_Message("LISTKEYS/paginated failed, status = %s", Status(status));
			return 1;
		}
		
		i = 0;
		while (result)
		{
			i++;
			if (i > num)
				break;

			startKey.Set(result->Key());
			Log_Trace("%s", startKey.buffer);
			result = result->Next(status);
		}
		
		if (i != num)
		{
			Log_Message("LISTKEYS/paginated failed (returned %d of %d)", i, num);
			return 1;
		}
		
		// list the next num items
		status = client.ListKeys(prefix, startKey, num, true);
		result = client.GetResult(status);
		if (status != KEYSPACE_OK || !result)
		{
			Log_Message("LISTKEYS/paginated failed, status = %s", Status(status));
			return 1;
		}
		
		i = 0;
		while (result)
		{
			i++;
			if (i > num)
				break;

			startKey.Set(result->Key());
			Log_Trace("%s", startKey.buffer);
			result = result->Next(status);
		}
		
		if (i != num)
		{
			Log_Message("LISTKEYS/paginated failed (returned %d of %d)", i, num);
			return 1;
		}
		
		Log_Message("LISTKEYS/paginated succeeded");
	}
	
	// paginated LISTKEYS test #2
	{
		num = NUM_TEST_KEYS;
		DynArray<128> prefix;
		DynArray<128> startKey;
		bool skip = false;
		int i;
		
		i = 0;
		prefix.Writef("test:");
		while (true)
		{
			int j = 10;
			status = client.ListKeys(prefix, startKey, j, skip);

			if (status != KEYSPACE_OK)
			{
				Log_Message("LISTKEYS/paginated2 failed (returned %d of %d), status = %s", i, num, Status(status));
				return 1;
			}
			
			result = client.GetResult(status);
			if (status != KEYSPACE_OK)
			{
				Log_Message("LISTKEYS/paginated2 failed (returned %d of %d), status = %s", i, num, Status(status));
				return 1;
			}
			
			if (!result)
				break;
			
			while (result)
			{
				i++;
				if (i > num)
					break;
				j--;
				startKey.Set(result->Key());
				//Log_Message("LISTKEYS/paginated2: %.*s", startKey.length, startKey.buffer);
				result = result->Next(status);
			}
			
			if (j != 0)
				break;
			
			skip = true;
			//Log_Message("LISTKEYS/paginated2: next startKey %.*s", startKey.length, startKey.buffer);
		}
		
		if (i != num)
		{
			Log_Message("LISTKEYS/paginated2 failed (returned %d of %d)", i, num);
			return 1;			
		}
		
		Log_Message("LISTKEYS/paginated2 succeeded");
	}
	
	// paginated LIST test #3
	{
		const char *MCULE_KEYS[] = {
			"AIWFTJJFRLPIRD-MRSUPTMICU",
			"APOHQRLKHMYCPF-QVUQFMIFCJ",
			"AXVOOBOVMQRASC-SREBMQDQCV",
			"AYZALFCBEKQGRT-UHFFFAOYAR",
			"BDCSBXZUWVUZCA-QVUQFMIFCJ",
			"BKWFEMTXXPTGLA-YDZHTSKRBJ",
			"BTEKPINUMIFNBM-VJSLDGLSCH",
			"BUVPXRLLVYZSRD-UYBDAZJACK",
			"BWRYEMXLQWFRBY-HPHMPNDVCT",
			"BZXAOUVLGYOFEG-FHGMOFAHCB",
			"CDABDEPCTYYMKV-CSKMVECVCL",
			"CMVXYUTUJOYOAR-PKSOQXRJCA",
			"CSIIRWICTOESQE-UFFVCSGVBO",
			"CZDLUSFSLSUPJN-PECIQRARCQ",
			"DGGYASFKJKXFGZ-LNNLXFCOCT",
			"DIQREXVGAWIWBR-UHFFFAOYAL",
			"DKQMKVKLJKRMSW-ZYMSVLFVCP",
			"DNDOHRFCVODJQJ-HPHMPNDVCW",
			"DTKHUBUAGGQZTK-LELJVTLKCB",
			"DYOPHMVJTYDODD-HPHMPNDVCL",
			"FDAFLYIGAPIPTG-CSKMVECVCQ",
			"FEXQYNVUMVIVGE-HXTKINSTCI",
			"FHETWNQRNKXWAN-MRSUPTMICC",
			"FMLTTXTUNXGTBH-QWOVJGMICA",
			"FPTBBFJJSPCSLZ-CSKMVECVCG",
			"FXUZDUAQIFWDOB-UHFFFAOYAW",
			"GGTKSBGQTCSOOX-PECIQRARCI",
			"GJESMWQDRWZUKC-MRSUPTMICW",
			"GKWVVVFFRZMMSC-QVUQFMIFCP",
			"GLGPSEIQGWMHBJ-KZFATGLACW",
			"HEHSZSBMWMFSPW-SDRQFZCRCW",
			"HFAXUZPFXVOFRG-JLGFQASFCJ",
			"HHHRZHVCTJGJMM-LQFNOIFHCO",
			"HHKCKALQPRFHMV-QWOVJGMICX",
			"HNOGSSUZDLBROJ-MDVJYLRGCM",
			"HYQGHDMXUYVPHG-KEBDBYFIBT",
		};
		DynArray<128> prefix;
		DynArray<128> startKey;
		bool skip = false;
		int num = SIZE(MCULE_KEYS);
		int i;
		
		prefix.Writef("mcule:");
		for (i = 0; i < num; i++)
		{
			key.Writef("%B%s", prefix.length, prefix.buffer, MCULE_KEYS[i]);
			status = client.Set(key, key);
			if (status != KEYSPACE_OK)
			{
				Log_Message("LISTKEYS/paginated3 Set() failed, status = %s", Status(status));
				return 1;
			}			
		}
		
		num = SIZE(MCULE_KEYS);
		i = 0;
		while (true)
		{
			int j = 10;
			status = client.ListKeys(prefix, startKey, j, skip);

			if (status != KEYSPACE_OK)
			{
				Log_Message("LISTKEYS/paginated3 failed (returned %d of %d), status = %s", i, num, Status(status));
				return 1;
			}
			
			result = client.GetResult(status);
			if (status != KEYSPACE_OK)
			{
				Log_Message("LISTKEYS/paginated3 failed (returned %d of %d), status = %s", i, num, Status(status));
				return 1;
			}
			
			if (!result)
				break;
			
			while (result)
			{
				i++;
				if (i > num)
					break;
				j--;
				startKey.Set(result->Key());
				//Log_Message("LISTKEYS/paginated3: %.*s", startKey.length, startKey.buffer);
				result = result->Next(status);
			}
			
			if (j != 0)
				break;
			
			skip = true;
			//Log_Message("LISTKEYS/paginated3: next startKey %.*s", startKey.length, startKey.buffer);
		}

		if (i != num)
		{
			Log_Message("LISTKEYS/paginated3 failed (returned %d of %d)", i, num);
			return 1;			
		}

		Log_Message("LISTKEYS/paginated3 succeeded");
	}
	
	// timeout test
	{
		key.Writef("test:0");
		status = client.Get(key);
		if (status != KEYSPACE_OK)
		{
			Log_Message("TimeoutTest failed, status = %s", Status(status));
			return 1;
		}

		Log_Message("TimeoutTest: waiting for %d secs", client.GetTimeout() + 1000);
		Sleep(client.GetTimeout() + 1000);
		
		status = client.Get(key);
		if (status != KEYSPACE_OK)
		{
			Log_Message("TimeoutTest failed, status = %s", Status(status));
			return 1;
		}
		
		Log_Message("TimeoutTest succeeded");
	}
	
	// PRUNE test
	{
		DynArray<128> prefix;
		
		prefix.Writef("test:");
		status = client.Prune(prefix);
		if (status != KEYSPACE_OK)
		{
			Log_Message("PRUNE failed, status = %s", Status(status));
			return 1;
		}
		
		Log_Message("PRUNE succeeded");
	}
	
	// LIST test for empty result
	{
		DynArray<128> prefix;
		DynArray<128> startKey;
		
		prefix.Writef("test:");
		status = client.ListKeys(prefix, startKey, 0, false);
		if (status != KEYSPACE_OK)
		{
			Log_Message("LISTKEYS/empty failed, status = %s", Status(status));
			return -1;
		}
		result = client.GetResult(status);
		if (status != KEYSPACE_OK || result != NULL)
		{
			Log_Message("LISTKEYS/empty failed, status = %s", Status(status));
			return -1;
		}
		
		Log_Message("LISTKEYS/emtpy succeeded");
	}

	// LIST test for empty result #2
	{
		DynArray<128> prefix;
		DynArray<128> startKey;
		
		key.Writef("test:0");
		status = client.Set(key, key);
		if (status != KEYSPACE_OK)
		{
			Log_Message("LISTKEYS/empty2 Set() failed, status = %s", Status(status));
			return -1;
		}
		
		prefix.Writef("test:");
		status = client.ListKeys(prefix, startKey, 0, true);
		if (status != KEYSPACE_OK)
		{
			Log_Message("LISTKEYS/empty2 status failed, status = %s", Status(status));
			return -1;
		}
		result = client.GetResult(status);
		if (status != KEYSPACE_OK || result != NULL)
		{
			Log_Message("LISTKEYS/empty2 result failed, status = %s", Status(status));
			return -1;
		}
		
		Log_Message("LISTKEYS/emtpy2 succeeded");
	}
	
	// limit SET test
	{
		int d = 1;
		uint64_t timeout = client.SetTimeout(3000);
		
		key.Fill('k', KEYSPACE_KEY_SIZE + d);
		value.Fill('v', KEYSPACE_VAL_SIZE + d);

		for (int i = -d; i <= d; i++)
		{
			for (int j = -d; j <= d; j++)
			{
				key.length = KEYSPACE_KEY_SIZE + i;
				value.length = KEYSPACE_VAL_SIZE + j;

				status = client.Set(key, value);
				if (status != KEYSPACE_OK)
				{
					if (key.length <= KEYSPACE_KEY_SIZE && value.length <= KEYSPACE_VAL_SIZE)
					{
						Log_Message("SET/limit failed, keySize = %d, valSize = %d", key.length, value.length);
						return 1;
					}
				}
				else
				{
					if (key.length > KEYSPACE_KEY_SIZE && value.length > KEYSPACE_VAL_SIZE)
					{
						Log_Message("SET/limit failed, keySize = %d, valSize = %d", key.length, value.length);
						return 1;
					}
				}
			}
		}
		
		client.SetTimeout(timeout);
		Log_Message("SET/limit succeeded");
	}

	return 0;
}

#ifndef TEST

int
main(int argc, char** argv)
{
	KeyspaceClientTest(argc, argv);

	return 0;
}

#endif
