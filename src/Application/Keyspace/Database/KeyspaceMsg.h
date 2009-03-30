#ifndef KEYSPACEMSG_H
#define KEYSPACEMSG_H

#include "System/Buffer.h"
#include "KeyspaceConsts.h"
#include "KeyspaceClient.h"

#define KEYSPACE_GET		'g'
#define KEYSPACE_SET		's'
#define KEYSPACE_TESTANDSET	't'
#define KEYSPACE_DELETE		'd'
#define KEYSPACE_INCREMENT	'i'

class KeyspaceMsg
{
public:
	char					type;
	ByteArray<KEY_SIZE>		key;
	ByteArray<VAL_SIZE>		value;
	ByteArray<VAL_SIZE>		test;
	
	
	void					Init(char type_);
	
	void					Get(ByteString key_);
	void					Set(ByteString key_, ByteString value_);
	void					TestAndSet(ByteString key_, ByteString test_, ByteString value_);
	void					Increment(ByteString key_);
	void					Delete(ByteString key_);

	bool					Read(ByteString& data, unsigned &nread);
	bool					Write(ByteString& data);

	bool					BuildFrom(KeyspaceOp* op);
};

#endif
