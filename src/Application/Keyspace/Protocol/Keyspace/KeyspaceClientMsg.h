#ifndef KEYSPACECLIENTMSG_H
#define KEYSPACECLIENTMSG_H

#include "System/Buffer.h"
#include "KeyspaceConsts.h"

#define KEYSPACECLIENT_GET			'g'
#define KEYSPACECLIENT_DIRTYGET		'd'
#define KEYSPACECLIENT_LIST			'l'
#define KEYSPACECLIENT_DIRTYLIST	'x'
#define KEYSPACECLIENT_SET			's'
#define KEYSPACECLIENT_TESTANDSET	't'
#define KEYSPACECLIENT_DELETE		'd'
#define KEYSPACECLIENT_INCREMENT	'i'

class KeyspaceClientMsg
{
public:
	char					type;
	ByteArray<KEY_SIZE>		key;
	ByteArray<VAL_SIZE>		value;
	ByteArray<VAL_SIZE>		test;
	uint64_t					count;
	
	
	void					Init(char type_);
	
	void					Get(ByteString key_);
	void					DirtyGet(ByteString key_);
	void					List(uint64_t count_);
	void					DirtyList(uint64_t count_);
	void					Set(ByteString key_, ByteString value_);
	void					TestAndSet(ByteString key_, ByteString test_, ByteString value_);
	void					Increment(ByteString key_);
	void					Delete(ByteString key_);

	bool					Read(ByteString& data, unsigned &nread);
	bool					Write(ByteString& data);
};

#endif
