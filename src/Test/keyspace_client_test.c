#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "Application/Keyspace/Client/keyspace_client.h"

#define Log_Trace(fmt, ...) printf("%s:%d: " fmt "\n", __func__, __LINE__, __VA_ARGS__)

void ignore_pipe_signal()
{
	sigset_t	sigset;
	
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGPIPE);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
}

int keyspace_client_test()
{
	const char *		nodes[] = {"127.0.0.1:7080", "127.0.0.1:7081", "127.0.01:7082"};
//	char				*nodes[] = {"127.0.0.1:7080"};
	keyspace_client_t	kc;
	const void *		pkey;
	char				key[128];
	unsigned			keylen;
	const void *		pval;
	char				val[128];
	unsigned			vallen;
	int					status;
	keyspace_result_t	kr;
	
	ignore_pipe_signal();
	
	kc = keyspace_client_create();
	
	status = keyspace_client_init(kc, sizeof(nodes) / sizeof(*nodes), nodes, 10000);
	if (status < 0)
		return 1;
		
	// set 100 keyvalues named user:
	status = keyspace_client_begin(kc);
	if (status == KEYSPACE_OK)
	{
		int i;
		for (i = 0; i < 100; i++)
		{
			keylen = snprintf(key, sizeof(key), "user:%d", i);
			vallen = snprintf(val, sizeof(val), "User %d", i);
			keyspace_client_set(kc, key, keylen, val, vallen, 0);
		}
		keyspace_client_submit(kc);
	}

	// get a key named "counter"
	keylen = snprintf(key, sizeof(key), "counter");
	keyspace_client_get(kc, key, keylen, 0);
	kr = keyspace_client_result(kc, &status);
	if (status == KEYSPACE_OK && kr != KEYSPACE_INVALID_RESULT)
	{
		pkey = keyspace_result_key(kr, &keylen);
		pval = keyspace_result_value(kr, &vallen);
		Log_Trace("Get result: key = %.*s, value = %.*s", keylen, pkey, vallen, pval);
		
		keyspace_result_close(kr);
	}

	keylen = snprintf(key, sizeof(key), "user:");		

	// list all keys starting with "user:"
	keyspace_client_list_keys(kc, key, keylen, NULL, 0, 0, 0, 0, 0);
	kr = keyspace_client_result(kc, &status);
	while (status == KEYSPACE_OK && kr != KEYSPACE_INVALID_RESULT)
	{
		pkey = keyspace_result_key(kr, &keylen);
		Log_Trace("List result: key = %.*s", keylen, pkey);

		kr = keyspace_result_next(kr, &status);
	}
	
	// list all keys and values starting with "user:" (List Key-Value Pairs)
	keylen = snprintf(key, sizeof(key), "user:");		
	keyspace_client_list_keyvalues(kc, key, keylen, NULL, 0, 0, 0, 0, 0);
	kr = keyspace_client_result(kc, &status);
	while (status == KEYSPACE_OK && kr != KEYSPACE_INVALID_RESULT)
	{
		pkey = keyspace_result_key(kr, &keylen);
		pval = keyspace_result_value(kr, &vallen);
		Log_Trace("List result: key = %.*s, value = %.*s", keylen, pkey, vallen, pval);

		kr = keyspace_result_next(kr, &status);
	}
	
	return 0;
}

#ifndef TEST

int
main(int argc, char** argv)
{
	keyspace_client_test();

	return 0;
}

#endif
