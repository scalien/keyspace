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
#define KEYSPACE_SET_EXPIRY		'x'
#define KEYSPACE_EXPIRE			'y'
#define KEYSPACE_REMOVE_EXPIRY	'z'

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
	uint64_t	expiryTime;
	
	void		Init(char type_);
	
	void		Set(ByteString key_, ByteString value_);
	void		TestAndSet(ByteString key_, 
				ByteString test_, ByteString value_);
	void		Add(ByteString key_, int64_t num_);
	void		Rename(ByteString key_, ByteString newKey_);
	void		Delete(ByteString key_);
	void		Remove(ByteString key_);
	void		Prune(ByteString prefix_);
	void		SetExpiry(ByteString key_, uint64_t expiryTime);
	void		Expire(ByteString key_, uint64_t expiryTime);
	void		RemoveExpiry(ByteString key_);

	bool		Read(ByteString& data, unsigned &nread);
	bool		Write(ByteString& data);

	bool		FromKeyspaceOp(KeyspaceOp* op);
};

#endif
