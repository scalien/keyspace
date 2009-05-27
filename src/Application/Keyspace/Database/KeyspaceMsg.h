#ifndef KEYSPACEMSG_H
#define KEYSPACEMSG_H

#include "System/Buffer.h"
#include "KeyspaceConsts.h"

#define KEYSPACE_SET			's'
#define KEYSPACE_TEST_AND_SET	't'
#define KEYSPACE_ADD			'a'
#define KEYSPACE_DELETE			'd'
#define KEYSPACE_PRUNE			'p'

class KeyspaceOp;

class KeyspaceMsg
{
public:
	char							type;
	ByteArray<KEYSPACE_KEY_SIZE>	key;
	ByteArray<KEYSPACE_VAL_SIZE>	value;
	ByteArray<KEYSPACE_VAL_SIZE>	test;
	ByteArray<KEYSPACE_VAL_SIZE>	prefix;
	int64_t							num;
	
	void							Init(char type_);
	
	void							Set(ByteString key_, ByteString value_);
	void							TestAndSet(ByteString key_, ByteString test_, ByteString value_);
	void							Add(ByteString key_, int64_t num_);
	void							Delete(ByteString key_);
	void							Prune(ByteString prefix_);

	bool							Read(ByteString& data, unsigned &nread);
	bool							Write(ByteString& data);

	bool							FromKeyspaceOp(KeyspaceOp* op);
};

#endif
