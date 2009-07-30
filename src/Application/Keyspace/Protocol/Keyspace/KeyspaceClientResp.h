#ifndef KEYSPACECLIENTRESP_H
#define KEYSPACECLIENTRESP_H

#define KEYSPACECLIENT_OK			'o'
#define KEYSPACECLIENT_FAILED		'f'
#define KEYSPACECLIENT_NOT_MASTER	'n'
#define KEYSPACECLIENT_LIST_ITEM	'i'
#define KEYSPACECLIENT_LISTP_ITEM	'j'
#define KEYSPACECLIENT_LIST_END		'.'

#include "System/Buffer.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"

class KeyspaceClientResp
{
public:
	char		type;
	uint64_t	cmdID;
	ByteString	key;
	ByteString	value;
	bool		sendValue;
	
	void		Ok(uint64_t cmdID_);
	void		Ok(uint64_t cmdID_, ByteString value_);
	void		Failed(uint64_t cmdID_);
	void		NotMaster(uint64_t cmdID_);
	void		ListItem(uint64_t cmdID_, ByteString key_);
	void		ListPItem(uint64_t cmdID_, ByteString key_, ByteString value_);
	void		ListEnd(uint64_t cmdID_);
	
	bool		Write(ByteString& data);
};

#endif
