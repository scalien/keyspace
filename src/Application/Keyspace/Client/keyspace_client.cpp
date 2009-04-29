#include "keyspace_client.h"

#include "KeyspaceClient.h"

keyspace_result_t
keyspace_client_result(keyspace_client_t kc, int *status)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;
	
	return (keyspace_result_t) client->GetResult(*status);
}

void
keyspace_result_close(keyspace_result_t kr)
{
	KeyspaceClient::Result *result = (KeyspaceClient::Result *) kr;

	result->Close();
}

keyspace_result_t
keyspace_result_next(keyspace_result_t kr, int *status)
{
	KeyspaceClient::Result *result = (KeyspaceClient::Result *) kr;
	
	return (keyspace_result_t) result->Next(*status);
}

int
keyspace_result_status(keyspace_result_t kr)
{
	KeyspaceClient::Result *result = (KeyspaceClient::Result *) kr;

	return result->Status();
}

const void *
keyspace_result_key(keyspace_result_t kr, unsigned *keylen)
{
	KeyspaceClient::Result *result = (KeyspaceClient::Result *) kr;
	const ByteString &key = result->Key();

	*keylen = key.length;
	return key.buffer;
}

const void *
keyspace_result_value(keyspace_result_t kr, unsigned *vallen)
{
	KeyspaceClient::Result *result = (KeyspaceClient::Result *) kr;
	const ByteString &val = result->Value();

	*vallen = val.length;
	return val.buffer;
}


keyspace_client_t
keyspace_client_create()
{
	return new KeyspaceClient();
}

void
keyspace_client_destroy(keyspace_client_t kc)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;

	delete client;
}

int
keyspace_client_init(keyspace_client_t kc, int nodec, char* nodev[], uint64_t timeout)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;
	
	return client->Init(nodec, nodev, timeout);
}

int
keyspace_client_connect_master(keyspace_client_t kc)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;

	return client->ConnectMaster();
}

int
keyspace_client_get_master(keyspace_client_t kc)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;
	
	return client->GetMaster();
}

int
keyspace_client_get_simple(keyspace_client_t kc, 
		const void *key_, unsigned keylen, 
		void *val_, unsigned vallen, 
		int dirty)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;
	const ByteString key(keylen, keylen, key_);
	ByteString val(vallen, 0, val_);
	
	return client->Get(key, val, dirty ? true : false);
}

int
keyspace_client_get(keyspace_client_t kc, 
		const void *key_, unsigned keylen, 
		int dirty)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;
	const ByteString key(keylen, keylen, key_);
	
	return client->Get(key, dirty ? true : false);
}

int
keyspace_client_list(keyspace_client_t kc, 
		const void *key_, unsigned keylen,
		uint64_t count,
		int dirty)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;
	const ByteString key(keylen, keylen, key_);
	
	return client->List(key, count, dirty ? true : false);
}

int
keyspace_client_listp(keyspace_client_t kc, 
		const void *key_, unsigned keylen,
		uint64_t count,
		int dirty)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;
	const ByteString key(keylen, keylen, key_);
	
	return client->ListP(key, count, dirty ? true : false);
}

int
keyspace_client_set(keyspace_client_t kc,
		const void *key_, unsigned keylen,
		const void *val_, unsigned vallen,
		int submit)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;
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
	KeyspaceClient *client = (KeyspaceClient *) kc;
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
	KeyspaceClient *client = (KeyspaceClient *) kc;
	const ByteString key(keylen, keylen, key_);

	return client->Add(key, num, *result, submit ? true : false);
}

int
keyspace_client_delete(keyspace_client_t kc,
		const void *key_, unsigned keylen,
		int submit)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;
	const ByteString key(keylen, keylen, key_);

	return client->Delete(key, submit ? true : false);
}

int
keyspace_client_begin(keyspace_client_t kc)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;
	
	return client->Begin();
}

int
keyspace_client_submit(keyspace_client_t kc)
{
	KeyspaceClient *client = (KeyspaceClient *) kc;

	return client->Submit();
}


