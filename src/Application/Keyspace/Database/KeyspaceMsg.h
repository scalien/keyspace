#ifndef KEYSPACEMSG_H
#define KEYSPACEMSG_H

#include "System/Buffer.h"
#include "KeyspaceConsts.h"

#define KEYSPACE_SET			's'
#define KEYSPACE_TEST_AND_SET	't'
#define KEYSPACE_ADD			'a'
#define KEYSPACE_DELETE			'd'
#define KEYSPACE_PRUNE			'p'
#define KEYSPACE_RENAME			'e'
#define KEYSPACE_REMOVE			'r'

class KeyspaceOp;

class KeyspaceMsg
{
typedef ByteArray<KEYSPACE_KEY_SIZE> KeyBuffer;
typedef ByteArray<KEYSPACE_VAL_SIZE> ValBuffer;
public:
	char		type;
	KeyBuffer	key;
	KeyBuffer	newKey;
	ValBuffer	value;
	ValBuffer	test;
	ValBuffer	prefix;
	int64_t		num;
	
	void		Init(char type_);
	
	void		Set(ByteString key_, ByteString value_);
	void		TestAndSet(ByteString key_, 
				ByteString test_, ByteString value_);
	void		Add(ByteString key_, int64_t num_);
	void		Rename(ByteString key_, ByteString newKey_);
	void		Delete(ByteString key_);
	void		Remove(ByteString key_);
	void		Prune(ByteString prefix_);

	bool		Read(const ByteString& data, unsigned &nread);
	bool		Write(ByteString& data) const;

	bool		FromKeyspaceOp(const KeyspaceOp* op);
};

#endif
