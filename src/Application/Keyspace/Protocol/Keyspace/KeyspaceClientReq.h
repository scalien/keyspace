#ifndef KEYSPACECLIENTREQ_H
#define KEYSPACECLIENTREQ_H

#include "System/Buffer.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"

#define KEYSPACECLIENT_GET_MASTER	'm'
#define KEYSPACECLIENT_GET			'g'
#define KEYSPACECLIENT_DIRTY_GET	'G'
#define KEYSPACECLIENT_LIST			'l'
#define KEYSPACECLIENT_DIRTY_LIST	'L'
#define KEYSPACECLIENT_LISTP		'p'
#define KEYSPACECLIENT_DIRTY_LISTP	'P'
#define KEYSPACECLIENT_SET			's'
#define KEYSPACECLIENT_TEST_AND_SET	't'
#define KEYSPACECLIENT_DELETE		'd'
#define KEYSPACECLIENT_PRUNE		'z'
#define KEYSPACECLIENT_ADD			'a'
#define KEYSPACECLIENT_REMOVE		'r'
#define KEYSPACECLIENT_RENAME		'e'
#define KEYSPACECLIENT_SUBMIT		'*'

class KeyspaceOp;

class KeyspaceClientReq
{
public:
	char			type;
	ByteString		key;
	ByteString		newKey;
	ByteString		value;
	ByteString		test;
	ByteString		prefix;
	uint64_t		cmdID;
	uint64_t		count;
	uint64_t		offset;
	int64_t			num;
	char			direction;
	
	void			Init();
	bool			Read(const ByteString& data);	
	bool			ToKeyspaceOp(KeyspaceOp* op);
};

#endif
