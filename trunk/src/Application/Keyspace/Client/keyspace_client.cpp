#include "keyspace_client.h"
#include "KeyspaceClient.h"

using namespace Keyspace;

keyspace_result_t
keyspace_client_result(keyspace_client_t kc, int *status)
{
	Client *client = (Client *) kc;
	
	return (keyspace_result_t) client->GetResult(*status);
}

void
keyspace_result_close(keyspace_result_t kr)
{
	Result *result = (Result *) kr;

	result->Close();
}

keyspace_result_t
keyspace_result_next(keyspace_result_t kr, int *status)
{
	Result *result = (Result *) kr;
	
	return (keyspace_result_t) result->Next(*status);
}

int
keyspace_result_status(keyspace_result_t kr)
{
	Result *result = (Result *) kr;

	return result->Status();
}

const void *
keyspace_result_key(keyspace_result_t kr, unsigned *keylen)
{
	Result *result = (Result *) kr;
	const ByteString &key = result->Key();

	*keylen = key.length;
	return key.buffer;
}

const void *
keyspace_result_value(keyspace_result_t kr, unsigned *vallen)
{
	Result *result = (Result *) kr;
	const ByteString &val = result->Value();

	*vallen = val.length;
	return val.buffer;
}


keyspace_client_t
keyspace_client_create()
{
	return new Client();
}

void
keyspace_client_destroy(keyspace_client_t kc)
{
	Client *client = (Client *) kc;

	delete client;
}

int
keyspace_client_init(keyspace_client_t kc, int nodec, const char* nodev[], uint64_t timeout)
{
	Client *client = (Client *) kc;
	
	return client->Init(nodec, nodev, timeout);
}

int
keyspace_client_get_master(keyspace_client_t kc)
{
	Client *client = (Client *) kc;
	
	return client->GetMaster();
}

int
keyspace_client_get_simple(keyspace_client_t kc, 
		const void *key_, unsigned keylen, 
		void *val_, unsigned vallen, 
		int dirty)
{
	Client *client = (Client *) kc;
	const ByteString key(keylen, keylen, key_);
	ByteString val(vallen, 0, val_);
	
	return client->Get(key, val, dirty ? true : false);
}

int
keyspace_client_get(keyspace_client_t kc, 
		const void *key_, unsigned keylen, 
		int dirty)
{
	Client *client = (Client *) kc;
	const ByteString key(keylen, keylen, key_);
	
	return client->Get(key, dirty ? true : false);
}

int
keyspace_client_list_keys(keyspace_client_t kc, 
		const void *key_, unsigned keylen,
		const void *start_key, unsigned sklen,
		uint64_t count,
		int skip,
		int dirty)
{
	Client *client = (Client *) kc;
	const ByteString key(keylen, keylen, key_);
	const ByteString sk(sklen, sklen, start_key);
	
	if (dirty)
		return client->DirtyListKeys(key, sk, count, skip ? true : false);
	else
		return client->ListKeys(key, sk, count, skip ? true : false);

}

int
keyspace_client_list_keyvalues(keyspace_client_t kc, 
		const void *key_, unsigned keylen,
		const void *start_key, unsigned sklen,
		uint64_t count,
		int skip,
		int dirty)
{
	Client *client = (Client *) kc;
	const ByteString key(keylen, keylen, key_);
	const ByteString sk(sklen, sklen, start_key);
	
	if (dirty)
		return client->DirtyListKeyValues(key, sk, count, skip ? true : false);
	else
		return client->ListKeyValues(key, sk, count, skip ? true : false);
}

int
keyspace_client_set(keyspace_client_t kc,
		const void *key_, unsigned keylen,
		const void *val_, unsigned vallen,
		int submit)
{
	Client *client = (Client *) kc;
	const ByteString key(keylen, keylen, key_);
	const ByteString value(vallen, vallen, val_);
	
	return client->Set(key, value, submit ? true : false);
}

int
keyspace_client_test_and_set(keyspace_client_t kc,
		const void *key_, unsigned keylen,
		const void *test_, unsigned testlen,
		const void *val_, unsigned vallen,
		int submit)
{
	Client *client = (Client *) kc;
	const ByteString key(keylen, keylen, key_);
	const ByteString test(testlen, testlen, test_);
	const ByteString val(vallen, vallen, val_);
	
	return client->TestAndSet(key, test, val, submit ? true : false);
}

int
keyspace_client_add(keyspace_client_t kc,
		const void *key_, unsigned keylen,
		int64_t num,
		int64_t *result,
		int submit)
{
	Client *client = (Client *) kc;
	const ByteString key(keylen, keylen, key_);

	return client->Add(key, num, *result, submit ? true : false);
}

int
keyspace_client_delete(keyspace_client_t kc,
		const void *key_, unsigned keylen,
		int submit)
{
	Client *client = (Client *) kc;
	const ByteString key(keylen, keylen, key_);

	return client->Delete(key, submit ? true : false);
}

int
keyspace_client_remove(keyspace_client_t kc,
		const void *key_, unsigned keylen,
		int submit)
{
	Client *client = (Client *) kc;
	const ByteString key(keylen, keylen, key_);
	
	return client->Remove(key, submit ? true : false);
}

int
keyspace_client_rename(keyspace_client_t kc,
		const void *from_, unsigned fromlen,
		const void *to_, unsigned tolen,
		int submit)
{
	Client *client = (Client *) kc;
	const ByteString from(fromlen, fromlen, from_);
	const ByteString to(tolen, tolen, to_);
	
	return client->Rename(from, to, submit ? true : false);
}

int
keyspace_client_prune(keyspace_client_t kc,
		const void *prefix_, unsigned prefixlen,
		int submit)
{
	Client *client = (Client *) kc;
	const ByteString prefix(prefixlen, prefixlen, prefix_);
	
	return client->Prune(prefix, submit ? true : false);
}

int
keyspace_client_begin(keyspace_client_t kc)
{
	Client *client = (Client *) kc;
	
	return client->Begin();
}

int
keyspace_client_submit(keyspace_client_t kc)
{
	Client *client = (Client *) kc;

	return client->Submit();
}


