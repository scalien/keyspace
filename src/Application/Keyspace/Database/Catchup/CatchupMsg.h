#ifndef CATCHUPMSG_H
#define CATCHUPMSG_H

#include "../KeyspaceConsts.h"
#include "System/Buffer.h"

#define KEY_VALUE			'k'
#define CATCHUP_COMMIT		'c'

class CatchupMsg
{
public:
	char type;
	ByteArray<KEYSPACE_KEY_SIZE>	key;
	ByteArray<KEYSPACE_VAL_SIZE>	value;
	uint64_t						paxosID;
	
	void							Init(char type_);
	void							KeyValue(ByteString& key_, ByteString& value_);
	void							Commit(uint64_t paxosID);

	bool							Read(ByteString data);
	bool							Write(ByteString& data);
};

#endif
