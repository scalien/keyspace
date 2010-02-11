/*
 * Keyspace C client API tests
 *
 */
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "Application/Keyspace/Client/keyspace_client.h"

// Test configuration
static const char *	NODES[] = {"127.0.0.1:7080", "127.0.0.1:7081", "127.0.01:7082"};
static const int	TIMEOUT = 10000;

#ifdef _WIN32
#include <windows.h>
#define __func__ __FUNCTION__
#define snprintf _snprintf
#endif

#define Log_Trace(fmt, ...) printf("%s:%d: " fmt "\n", __func__, __LINE__, __VA_ARGS__)

void ignore_pipe_signal()
{
#ifndef _WIN32
	sigset_t	sigset;
	
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGPIPE);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
#endif
}

// for test functions we use the Unix return values:
// 0 for success
// anything else means failure
#define TEST_SUCCESS	0
#define TEST_FAILURE	Log_Trace("%s", "failure"), 1
#define TEST_CALL(testfn) { int status; Log_Trace("%s: testing", #testfn); status = testfn; Log_Trace("%s: %s", #testfn, status == TEST_SUCCESS ? "success" : "failure"); if (status != TEST_SUCCESS) return 1;}

// timing stuff
static const char* timeout_funcname = NULL;
static void timeout_func(void);
#ifdef _WIN32
static DWORD threadID;
static HANDLE threadHandle;
static int threadTimeout;
// TODO cancel previous thread if there is any
#define TEST_TIMEOUT_CALL(testfn, timeout) { timeout_funcname = #testfn; threadTimeout = timeout; threadHandle = CreateThread(NULL, 0, TimeoutThread, 0, 0, &threadID); testfn; TerminateThread(threadHandle, 0); timeout_funcname = NULL; }
DWORD WINAPI TimeoutThread(LPVOID pv)
{
	void timeout_func();
	Sleep(threadTimeout);
	if (timeout_funcname && threadID == GetCurrentThreadId())
		timeout_func();	

	return 0;
}
#else
#include <unistd.h>
#define TEST_TIMEOUT_CALL(testfn, timeout) { timeout_funcname = #testfn; signal(SIGALRM, sigalrm); alarm((int)(timeout / 1000.0 + 0.5)); testfn; alarm(0); }
static void sigalrm(int sig)
{
	(void) sig;
	timeout_func();
}
#endif

static void timeout_func()
{
	printf("%s: failure", timeout_funcname);
	exit(1);
}

// any command before init must fall
static int test_init_with_commands(keyspace_client_t kc)
{
	int					status;
	const char			key[] = "key";
	uint64_t			ures;
	int64_t				res;
	
	status = keyspace_client_get_master(kc);
	if (status != KEYSPACE_API_ERROR)
		return TEST_FAILURE;

	// this one is different
	status = keyspace_client_get_simple(kc, key, sizeof(key), NULL, 0, 0);
	if (status >= 0)
		return TEST_FAILURE;
	
	status = keyspace_client_get(kc, key, sizeof(key), 0);
	if (status != KEYSPACE_API_ERROR)
		return TEST_FAILURE;
	
	status = keyspace_client_count(kc, &ures, key, sizeof(key), key, sizeof(key), 0, 0, 0, 0);
	if (status != KEYSPACE_API_ERROR)
		return TEST_FAILURE;
	
	status = keyspace_client_list_keys(kc, key, sizeof(key), key, sizeof(key), 0, 0, 0, 0);
	if (status != KEYSPACE_API_ERROR)
		return TEST_FAILURE;

	status = keyspace_client_list_keyvalues(kc, key, sizeof(key), key, sizeof(key), 0, 0, 0, 0);
	if (status != KEYSPACE_API_ERROR)
		return TEST_FAILURE;

	// only submitted set return with error, the unsubmitted gets queued
	status = keyspace_client_set(kc, key, sizeof(key), key, sizeof(key), 1);
	if (status != KEYSPACE_API_ERROR)
		return TEST_FAILURE;

	status = keyspace_client_test_and_set(kc, key, sizeof(key), key, sizeof(key), key, sizeof(key), 1);
	if (status != KEYSPACE_API_ERROR)
		return TEST_FAILURE;

	status = keyspace_client_add(kc, key, sizeof(key), 0, &res, 1);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;

	status = keyspace_client_delete(kc, key, sizeof(key), 1);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;

	status = keyspace_client_remove(kc, key, sizeof(key), 1);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;

	status = keyspace_client_rename(kc, key, sizeof(key), key, sizeof(key), 1);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;

	status = keyspace_client_prune(kc, key, sizeof(key), 1);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;

	return TEST_SUCCESS;
}

// any operation before init must fall
static int test_init()
{
	keyspace_client_t	kc;
	keyspace_result_t	kr;
	int					status;
	
	kc = keyspace_client_create();
	
	status = keyspace_client_submit(kc);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;
	
	status = keyspace_client_begin(kc);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;
	
	status = keyspace_client_cancel(kc);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;

	kr = keyspace_client_result(kc);
	if (kr != KEYSPACE_INVALID_RESULT)
		return TEST_FAILURE;

	TEST_CALL(test_init_with_commands(kc));

	// test init with invalid arguments
	status = keyspace_client_init(kc, 0, NULL);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;
	
	status = keyspace_client_init(kc, sizeof(NODES) / sizeof(*NODES), NODES);
	if (status != KEYSPACE_SUCCESS)
		return TEST_FAILURE;
	
	keyspace_client_destroy(kc);
	
	return TEST_SUCCESS;
}

static int test_badconnect()
{
	keyspace_client_t	kc;
	int					status;
	const char*			nodes[] = {"127.0.0.0:7080"}; // Note that this is a bad address!
	const char			key[] = "key";
	
	kc = keyspace_client_create();
	status = keyspace_client_init(kc, sizeof(nodes) / sizeof(*nodes), nodes);
	if (status != KEYSPACE_SUCCESS)
		return TEST_FAILURE;
		
	status = keyspace_client_set_global_timeout(kc, TIMEOUT);
	if (status != KEYSPACE_SUCCESS)
		return TEST_FAILURE;
	
	TEST_TIMEOUT_CALL(status = keyspace_client_set(kc, key, sizeof(key), key, sizeof(key), 1), TIMEOUT + 1000);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;
	
	if (status == KEYSPACE_API_ERROR)
		return TEST_FAILURE;
	
	keyspace_client_destroy(kc);
	
	return TEST_SUCCESS;
}

static int test_invalid_result()
{
	keyspace_result_t	kr;
	int					status;
	const void*			p;
	unsigned			len;

	kr = KEYSPACE_INVALID_RESULT;
		
	status = keyspace_result_transport_status(kr);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;

	status = keyspace_result_key(kr, &p, NULL);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;
	
	len = 0x12345678;
	status = keyspace_result_key(kr, &p, &len);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;
	
	status = keyspace_result_value(kr, &p, NULL);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;
		
	len = 0x12345678;
	status = keyspace_result_value(kr, &p, &len);
	if (status == KEYSPACE_SUCCESS)
		return TEST_FAILURE;
	
	keyspace_result_close(kr);
	
	return TEST_SUCCESS;
}


int keyspace_client_basic_test()
{
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
	
	status = keyspace_client_init(kc, sizeof(NODES) / sizeof(*NODES), NODES);
	if (status < 0)
		return TEST_FAILURE;
		
	status = keyspace_client_set_global_timeout(kc, TIMEOUT);
	if (status < 0)
		return TEST_FAILURE;
	
		
	// set 100 keyvalues named user:
	status = keyspace_client_begin(kc);
	if (status == KEYSPACE_SUCCESS)
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
	kr = keyspace_client_result(kc);
	if (kr != KEYSPACE_INVALID_RESULT)
	{
		status = keyspace_result_key(kr, &pkey, &keylen);
		status = keyspace_result_value(kr, &pval, &vallen);
		Log_Trace("Get result: key = %.*s, value = %.*s", keylen, (char*) pkey, vallen, (char*) pval);
		
		keyspace_result_close(kr);
	}

	keylen = snprintf(key, sizeof(key), "user:");		

	// list all keys starting with "user:"
	keyspace_client_list_keys(kc, key, keylen, NULL, 0, 0, 0, 0, 0);
	kr = keyspace_client_result(kc);
	while (kr != KEYSPACE_INVALID_RESULT)
	{
		status = keyspace_result_key(kr, &pkey, &keylen);
		Log_Trace("List result: key = %.*s", keylen, (char*) pkey);

		if (keyspace_result_is_end(kr))
			break;
			
		keyspace_result_next(kr);
	}
	
	// list all keys and values starting with "user:" (List Key-Value Pairs)
	keylen = snprintf(key, sizeof(key), "user:");		
	keyspace_client_list_keyvalues(kc, key, keylen, NULL, 0, 0, 0, 0, 0);
	kr = keyspace_client_result(kc);
	while (kr != KEYSPACE_INVALID_RESULT)
	{
		status = keyspace_result_key(kr, &pkey, &keylen);
		status = keyspace_result_value(kr, &pval, &vallen);
		Log_Trace("List result: key = %.*s, value = %.*s", keylen, (char*) pkey, vallen, (char*) pval);

		if (keyspace_result_is_end(kr))
			break;
			
		keyspace_result_next(kr);
	}
	
	return TEST_SUCCESS;
}

int keyspace_client_test()
{
//	TEST_CALL(keyspace_client_basic_test());
	TEST_CALL(test_invalid_result());
	TEST_CALL(test_init());
	TEST_CALL(test_badconnect());
	
	return TEST_SUCCESS;
}

#ifdef CLIENT_TEST

int
main(int argc, char** argv)
{
	return keyspace_client_test();
}

#endif
