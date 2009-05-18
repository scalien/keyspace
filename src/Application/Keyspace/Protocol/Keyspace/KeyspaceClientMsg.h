#ifndef KEYSPACECLIENTMSG_H
#define KEYSPACECLIENTMSG_H

#include "System/Buffer.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"

#define KEYSPACECLIENT_GETMASTER	'm'
#define KEYSPACECLIENT_GET			'g'
#define KEYSPACECLIENT_DIRTYGET		'G'
#define KEYSPACECLIENT_LIST			'l'
#define KEYSPACECLIENT_DIRTYLIST	'L'
#define KEYSPACECLIENT_LISTP		'p'
#define KEYSPACECLIENT_DIRTYLISTP	'P'
#define KEYSPACECLIENT_SET			's'
#define KEYSPACECLIENT_TESTANDSET	't'
#define KEYSPACECLIENT_DELETE		'd'
#define KEYSPACECLIENT_PRUNE		'z'
#define KEYSPACECLIENT_ADD			'a'
#define KEYSPACECLIENT_SUBMIT		'*'

class KeyspaceOp;

class KeyspaceClientReq
{
public:
	char			type;
	ByteBuffer		key;
	ByteBuffer		value;
	ByteBuffer		test;
	uint64_t		cmdID;
	int64_t			num;
	uint64_t		count;
	
	
	void			Init(char type_);
	
	bool			GetMaster(uint64_t cmdID_);	
	bool			Get(uint64_t cmdID_, ByteString key_);
	bool			DirtyGet(uint64_t cmdID_, ByteString key_);
	bool			List(uint64_t cmdID_, ByteString prefix_, uint64_t count_);
	bool			DirtyList(uint64_t cmdID_, ByteString prefix_, uint64_t count_);
	bool			ListP(uint64_t cmdID_, ByteString prefix_, uint64_t count_);
	bool			DirtyListP(uint64_t cmdID_, ByteString prefix_, uint64_t count_);
	bool			Set(uint64_t cmdID_, ByteString key_, ByteString value_);
	bool			TestAndSet(uint64_t cmdID_, ByteString key_, ByteString test_, ByteString value_);
	bool			Add(uint64_t cmdID_, ByteString key_, int64_t num_);
	bool			Delete(uint64_t cmdID_, ByteString key_);
	bool			Prune(uint64_t cmdID_, ByteString prefix_);
	bool			Submit();

	bool			Read(ByteString data);
	
	bool			ToKeyspaceOp(KeyspaceOp* op);
};


#define KEYSPACECLIENT_OK			'o'
#define KEYSPACECLIENT_FAILED		'f'
#define KEYSPACECLIENT_NOTMASTER	'n'
#define KEYSPACECLIENT_LISTITEM		'i'
#define KEYSPACECLIENT_LISTPITEM	'j'
#define KEYSPACECLIENT_LISTEND		'.'

class KeyspaceClientResp
{
public:
	char			type;
	uint64_t		cmdID;
	ByteString		key;
	ByteString		value;
	
	void			Ok(uint64_t cmdID_);
	void			Ok(uint64_t cmdID_, ByteString value_);
	void			Failed(uint64_t cmdID_);
	void			NotMaster(uint64_t cmdID_);
	void			ListItem(uint64_t cmdID_, ByteString key_);
	void			ListPItem(uint64_t cmdID_, ByteString key_, ByteString value_);
	void			ListEnd(uint64_t cmdID_);
	
	bool			Write(ByteString& data);
};

#endif
