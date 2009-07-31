#ifndef CATCHUPMSG_H
#define CATCHUPMSG_H

#include "Application/Keyspace/Database/KeyspaceConsts.h"
#include "System/Buffer.h"

#define CATCHUP_KEY_VALUE	'k'
#define CATCHUP_COMMIT		'c'

class CatchupMsg
{
typedef ByteArray<KEYSPACE_KEY_SIZE> KeyBuffer; 
typedef ByteArray<KEYSPACE_VAL_SIZE> ValBuffer; 
public:
	char		type;
	KeyBuffer	key;
	ValBuffer	value;
	uint64_t	paxosID;
	
	void		Init(char type_);
	void		KeyValue(ByteString& key_, ByteString& value_);
	void		Commit(uint64_t paxosID);

	bool		Read(const ByteString& data);
	bool		Write(ByteString& data);
};

#endif
