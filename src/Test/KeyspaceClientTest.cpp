#include "Application/Keyspace/Client/KeyspaceClient.h"
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
	char			*nodes[] = {"127.0.0.1:7080", "127.0.0.1:7081", "127.0.01:7082"};
//	char			*nodes[] = {"127.0.0.1:7080"};
	KeyspaceClient	client;
	int				master;
	DynArray<128>	key;
	DynArray<128>	value;
	int64_t			num;
	int				status;
	KeyspaceClient::Result* result;
	
	IgnorePipeSignal();
	
	status = client.Init(SIZE(nodes), nodes, 10000);
	if (status < 0)
		return 1;
	
	// all the operations we do below are safe (not dirty) or write operations
	// so we connect to the master
	status = client.ConnectMaster();
	if (status < 0)
		return 1;
	
	// set 100 keyvalues named user:
	status = client.Begin();
	if (status == KEYSPACE_OK)
	{
		for (int i = 0; i < 100; i++)
		{
			key.Printf("user:%d", i);
			value.Printf("User %d", i);
			client.Set(key, value, false);
		}
		client.Submit();
	}

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

int
main(int, char**)
{
	KeyspaceClientTest();

	return 0;
}
