#include "Application/Keyspace/Client/KeyspaceClient.h"

int KeyspaceClientTest()
{
	char			*nodes[] = {"127.0.0.1:7080", "127.0.0.1:7081"};
	KeyspaceClient	client(SIZE(nodes), nodes, 0);
	int				master;
	DynArray<128>	key;
	DynArray<128>	value;
	int64_t			num;
	int				status;
	KeyspaceClient::Result* result;
	
//	master = client.GetMaster();
	client.ConnectMaster();
/*	
	key.Printf("counter");
	value.Printf("0");
	client.Set(key, value);
	client.DirtyGet(key, value);
	
	client.Add(key, 1, num);
	
	key.Clear();
	client.DirtyList(key);
*/
	client.Begin();
	for (int i = 0; i < 100; i++)
	{
		key.Printf("user:%d", i);
		value.Printf("User %d", i);
		client.Set(key, value, false);
	}
	client.Submit();


	key.Printf("counter");
	client.Get(key);
	result = client.GetResult(status);
	if (result)
	{
		Log_Trace("result key = %.*s, value = %.*s", result->Key().length, result->Key().buffer, result->Value().length, result->Value().buffer);
		result->Close();
	}

	key.Printf("user:");		

	client.List(key);
	result = client.GetResult(status);
	while (result)
	{
		Log_Trace("result key = %.*s, value = %.*s", result->Key().length, result->Key().buffer, result->Value().length, result->Value().buffer);		
		result = result->Next(status);
	}
	
	client.ListP(key);
	result = client.GetResult(status);
	while (result)
	{
		Log_Trace("result key = %.*s, value = %.*s", result->Key().length, result->Key().buffer, result->Value().length, result->Value().buffer);		
		result = result->Next(status);
	}
	
	return 0;
}
