#include "Application/Keyspace/Client/KeyspaceClient2.h"
#include "System/Stopwatch.h"
#include <signal.h>

void IgnorePipeSignal()
{
	sigset_t	sigset;
	
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGPIPE);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
}

int KeyspaceClientTest()
{
//	char			*nodes[] = {"127.0.0.1:7080", "127.0.0.1:7081", "127.0.01:7082"};
	char			*nodes[] = {"127.0.0.1:7080"};
	Keyspace::Client client;
	int				master;
	DynArray<128>	key;
	DynArray<1024>	value;
	DynArray<36>	reference;
	int64_t			num;
	int				status;
	Keyspace::Result* result;
	Stopwatch		sw;
	
	IgnorePipeSignal();

	Log_SetTrace(true);
	Log_SetTimestamping(true);
	
	status = client.Init(SIZE(nodes), nodes, 10000);
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
	
	
/*	
	status = client.Begin();
	if (status == KEYSPACE_OK)
	{		
		Log_SetTrace(false);
		num = 10000;

		value.length = 1000;
		for (int i = 0; i < 1000; i++)
			value.buffer[i] = '$';

		for (int i = 0; i < num; i++)
		{
			key.Writef("user:%d", i);
//			value.Printf("User %d", i);
			client.Set(key, value, false);
		}
		sw.Start();
		status = client.Submit();		
		sw.Stop();
		Log_SetTrace(true);
		if (status == KEYSPACE_OK)
			Log_Trace("write/sec = %lf", num / (sw.elapsed / 1000.0));
		else
			Log_Trace("failed, status = %d", status);
	}
*/
	key.Writef("");
	status = client.List(key);
	result = client.GetResult(status);
	while (result)
	{
		Log_Trace("%.*s", result->Key().length, result->Key().buffer);
		result = result->Next(status);
	}
	

	return 0;

	sw.Start();
	num = 100000;
	for (int i = 0; i < num; i++)
	{
		key.Printf("user:%d", i);
		value.Clear();
		client.DirtyGet(key, value);
	}
	sw.Stop();
	Log_SetTrace(true);
	Log_Trace("read/sec = %lf", num / (sw.elapsed / 1000.0));
	
	return 0;

	// get a key named "counter"
	key.Printf("counter");
	client.Get(key);
	result = client.GetResult(status);
	if (status == KEYSPACE_OK && result)
	{
		Log_Trace("Get result: key = %.*s, value = %.*s", result->Key().length, result->Key().buffer,
				  result->Value().length, result->Value().buffer);
		result->Close();
	}

	key.Printf("user:");		

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
main(int, char**)
{
	KeyspaceClientTest();

	return 0;
}

#endif
