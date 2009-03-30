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
		DIRTY_GET,
		GET,
		SET,
		TEST_AND_SET,
		INCREMENT,
		DELETE
	};
	
	Type					type;
	ByteBuffer				key;
	ByteBuffer				value;
	ByteBuffer				test;
	
	KeyspaceClient*			client;
	
	void Free()
	{
		key.Free();
		value.Free();
		test.Free();
	}
};

#endif
