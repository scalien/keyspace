#ifndef CATCHUPMSG_H
#define CATCHUPMSG_H

#include "KeyspaceConsts.h"
#include "System/Buffer.h"

#define KEY_VALUE			'k'
#define CATCHUP_COMMIT		'c'

class CatchupMsg
{
public:
	char type;
	ByteArray<KEY_SIZE>	key;
	ByteArray<VAL_SIZE>	value;
	
	ulong64				paxosID;
	
	void				Init(char type_);
		
	void				KeyValue(ByteString& key_, ByteString& value_);
	
	void				Commit(ulong64 paxosID);
	
	bool				Read(ByteString data);
	
	bool				Write(ByteString& data);
};

#endif
