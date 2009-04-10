#ifndef KEYSPACEMSG_H
#define KEYSPACEMSG_H

#include "System/Buffer.h"
#include "KeyspaceConsts.h"

#define KEYSPACE_GET		'g'
#define KEYSPACE_SET		's'
#define KEYSPACE_TESTANDSET	't'
#define KEYSPACE_DELETE		'd'
#define KEYSPACE_ADD		'a'

class KeyspaceOp;

class KeyspaceMsg
{
public:
	char					type;
	ByteArray<KEY_SIZE>		key;
	ByteArray<VAL_SIZE>		value;
	ByteArray<VAL_SIZE>		test;
	int64_t					addNum;
	
	
	void					Init(char type_);
	
	void					Get(ByteString key_);
	void					Set(ByteString key_, ByteString value_);
	void					TestAndSet(ByteString key_, ByteString test_, ByteString value_);
	void					Add(ByteString key_, int64_t addNum_);
	void					Delete(ByteString key_);

	bool					Read(ByteString& data, unsigned &nread);
	bool					Write(ByteString& data);

	bool					BuildFrom(KeyspaceOp* op);
};

#endif
