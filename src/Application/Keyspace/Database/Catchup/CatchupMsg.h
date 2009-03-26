#ifndef CATCHUPMSG_H
#define CATCHUPMSG_H

#include "KeyspaceConsts.h"
#include "System/Buffer.h"

#define KEY_VALUE			'0'
#define DB_COMMAND			'1'
#define CATCHUP_COMMIT		'2'
#define CATCHUP_ROLLBACK	'3'

class CatchupMsg
{
public:
	char type;
	ByteArray<KEY_SIZE>	key;
	ByteArray<VAL_SIZE>	value;
	
	ulong64				paxosID;
	ByteArray<VAL_SIZE>	dbCommand;
	
	
	void				Init(char type_);
		
	void				KeyValue(ByteString& key_, ByteString& value_);
	
	void				DBCommand(ulong64 paxosID_, ByteString& dbCommand_);
	
	void				Commit();
	
	void				Rollback();
	
	bool				Read(ByteString data);
	
	bool				Write(ByteString& data);
};

#endif
