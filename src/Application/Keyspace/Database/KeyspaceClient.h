#ifndef KEYSPACECLIENT_H
#define KEYSPACECLIENT_H

#include "System/Buffer.h"

class KeyspaceOp;


class KeyspaceClient
{
public:
	virtual	void	OnComplete(KeyspaceOp* op, bool status) = 0;
};


class KeyspaceOp
{
public:
	enum Type
	{
		GET,
		SET,
		TEST_AND_SET
	};
	
	Type					type;
	ByteString				key;
	ByteString				value;
	ByteString				test;
	
	KeyspaceClient*			client;
};

#endif
