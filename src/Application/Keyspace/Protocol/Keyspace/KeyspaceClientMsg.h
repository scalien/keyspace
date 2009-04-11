#ifndef KEYSPACECLIENTMSG_H
#define KEYSPACECLIENTMSG_H

#include "System/Buffer.h"
#include "KeyspaceConsts.h"

#define KEYSPACECLIENT_GETMASTER	'm'
#define KEYSPACECLIENT_GET			'g'
#define KEYSPACECLIENT_DIRTYGET		'G'
#define KEYSPACECLIENT_LIST			'l'
#define KEYSPACECLIENT_DIRTYLIST	'L'
#define KEYSPACECLIENT_SET			's'
#define KEYSPACECLIENT_TESTANDSET	't'
#define KEYSPACECLIENT_DELETE		'd'
#define KEYSPACECLIENT_ADD			'a'
#define KEYSPACECLIENT_SUBMIT		'*'

class KeyspaceOp;

class KeyspaceClientReq
{
public:
	char					type;
	ByteBuffer				key;
	ByteBuffer				value;
	ByteBuffer				test;
	int64_t					num;
	uint64_t				count;
	
	
	void					Init(char type_);
	
	bool					GetMaster();	
	bool					Get(ByteString key_);
	bool					DirtyGet(ByteString key_);
	bool					List(ByteString prefix_, uint64_t count_);
	bool					DirtyList(ByteString prefix_, uint64_t count_);
	bool					Set(ByteString key_, ByteString value_);
	bool					TestAndSet(ByteString key_, ByteString test_, ByteString value_);
	bool					Add(ByteString key_, int64_t num_);
	bool					Delete(ByteString key_);
	bool					Submit();

	bool					Read(ByteString data);
	bool					Write(ByteString& data);
	
	bool					ToKeyspaceOp(KeyspaceOp* op);
};


#define KEYSPACECLIENT_OK			'o'
#define KEYSPACECLIENT_NOTFOUND		'n'
#define KEYSPACECLIENT_FAILED		'f'
#define KEYSPACECLIENT_LISTEND		'.'

class KeyspaceClientResp
{
public:
	char					type;
	ByteString				value;
	
	void					OK();
	void					OK(ByteString value_);
	void					NotFound();
	void					Failed();
	void					ListEnd();
	
	bool					Write(ByteString& data);
};

#endif
