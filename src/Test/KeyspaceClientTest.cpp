#include "Application/Keyspace/Client/KeyspaceClient.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"
#include "System/Stopwatch.h"
#include "System/Config.h"
#include "TestConfig.h"
#include <signal.h>
#include <float.h>

using namespace Keyspace;
extern "C" int keyspace_client_test();
int KeyspaceClientTestSuite(Keyspace::Client& client);
int DatabaseSetTest(TestConfig& conf);

#ifdef PLATFORM_WINDOWS
void IgnorePipeSignal() {}
#else
void IgnorePipeSignal()
{
	sigset_t	sigset;
	
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGPIPE);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
}
#endif

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

	conf.SetKeySize(atoi(conf.argv[3]));
	conf.SetValueSize(atoi(conf.argv[4]), false);

	Log_Message("Test type = %s, keySize = %d, valueSize = %d",
			conf.typeString, conf.keySize, conf.valueSize);

	client.DistributeDirty(true);
	
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
	static uint64_t d = Now();

	assert(bs.size >= length);

	for (i = 0; i < length; i++) {
			// more about why these numbers were chosen:
			// http://en.wikipedia.org/wiki/Linear_congruential_generator
			d = (d * 1103515245UL + 12345UL) >> 2;
			bs.buffer[i] = set[d % setsize];
	}

	bs.length = length;
}

int KeyspaceClientSetTest(Keyspace::Client& client, TestConfig& conf)
{
	int			status;
	int			numTest;
	Stopwatch	sw;

	//return DatabaseSetTest(conf);


	if (conf.argc < 5)
	{
		Log_Message("\n\tusage: %s <keySize> <valueSize>", conf.typeString);
		return -1;
	}
	
	conf.SetKeySize(atoi(conf.argv[3]));
	conf.SetValueSize(atoi(conf.argv[4]));

	Log_Message("Test type = %s, keySize = %d, valueSize = %d",
			conf.typeString, conf.keySize, conf.valueSize);
	
	numTest = conf.datasetTotal / conf.valueSize;
	for (int i = 0; i < numTest; i++)
	{
		conf.key.Reallocate(conf.keySize, false);
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
	
	double mbps = (conf.valueSize + conf.keySize) * numTest / (sw.elapsed / 1000.0) / 1000000;
	Log_Message("Test succeeded, %s/sec = %lf (num = %d, elapsed = %ld, thruput = %lf MB/s)", conf.typeString, numTest / (sw.elapsed / 1000.0), numTest, sw.elapsed, mbps);
	
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

	if (conf.argc < 6)
	{
		Log_Message("\n\tusage: %s <keySize> <valueSize> <numTest>", conf.typeString);
		return -1;
	}
	
	conf.SetKeySize(atoi(conf.argv[3]));
	conf.SetValueSize(atoi(conf.argv[4]));
	numTest = atoi(conf.argv[5]);

	Log_Message("Test type = %s, keySize = %d, valueSize = %d, numTest = %d",
			conf.typeString, conf.keySize, conf.valueSize, numTest);
	
	for (int i = 0; i < numTest; i++)
	{
		conf.key.Reallocate(conf.keySize, false);
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
		else
			return 1;
			
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
	int					logTargets;


	if (argc < 3)
	{
		Log_Message("usage:\n\t%s <configFile> <command>", argv[0]);
		Log_Message("\n\t\tcommand can be any of get, dirtyget, set, rndset, list, listp, getlatency, dirtygetlatency, setlatency, suite, api\n");
		return 1;
	}

	IgnorePipeSignal();
	Config::Init(argv[1]);

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
	Log_SetTimestamping(true);

	nodec = Config::GetListNum("keyspace.endpoints");
	if (nodec <= 0)
	{
		nodec = SIZE(LOCAL_NODES);
		nodes = LOCAL_NODES;
	}
	else
	{
		nodes = new const char*[nodec];
		for (int i = 0; i < nodec; i++)
			nodes[i] = Config::GetListValue("keyspace.endpoints", i, NULL);
	}
	
	timeout = Config::GetIntValue("keyspace.timeout", 10000);
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
		
	if (testConf.type == TestConfig::SET)
		return KeyspaceClientSetTest(client, testConf);
	if (testConf.type == TestConfig::LIST || testConf.type == TestConfig::LISTP)
		return KeyspaceClientListTest(client, testConf);
	if (testConf.type == TestConfig::GETLATENCY || testConf.type == TestConfig::DIRTYGETLATENCY || testConf.type == TestConfig::SETLATENCY)
		return KeyspaceClientLatencyTest(client, testConf);
	if (testConf.type == TestConfig::GET)
		return KeyspaceClientGetTest(client, testConf);
	if (testConf.type == TestConfig::API)
		return keyspace_client_test();

	Log_Message("no such test: %s", testConf.typeString);
		
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

	Log_Message("SUITE starting");
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

	// cancel test
	{
		status = client.Begin();
		if (status != KEYSPACE_OK)
		{
			Log_Message("CancelTest Begin failed, status = %s", Status(status));
			return 1;
		}
		
		for (int i = 0; i < num; i++)
		{
			key.Writef("test:%d", i);
			status = client.Get(key, true, false);
			if (status != KEYSPACE_OK)
			{
				Log_Message("CancelTest Get failed, status = %s", Status(status));
				return 1;
			}		
		}
		
		status = client.Cancel();
		if (status != KEYSPACE_OK)
		{
			Log_Message("CancelTest Cancel failed, status = %s", Status(status));
			return 1;
		}
		
		Log_Message("CancelTest succeeded");
	}

	// timeout test
	{
		key.Writef("test:0");
		status = client.Set(key, key);
		if (status != KEYSPACE_OK)
		{
			Log_Message("TimeoutTest Set failed, status = %s", Status(status));
			return 1;
		}
		
		status = client.Get(key);
		if (status != KEYSPACE_OK)
		{
			Log_Message("TimeoutTest failed, status = %s", Status(status));
			return 1;
		}

		Log_Message("TimeoutTest: waiting for %d usecs", client.GetTimeout() + 1000);
		MSleep(client.GetTimeout() + 1000);
		
		status = client.Get(key);
		if (status != KEYSPACE_OK)
		{
			Log_Message("TimeoutTest failed, status = %s", Status(status));
			return 1;
		}
		
		Log_Message("TimeoutTest succeeded");
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
	int ret = 0;
	
	ret = KeyspaceClientTest(argc, argv);

#if defined(WIN32) && defined(_DEBUG)
	system("pause");
#endif

	return ret;
}

#endif
