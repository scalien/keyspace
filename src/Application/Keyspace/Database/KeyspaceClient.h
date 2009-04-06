#ifndef KEYSPACECLIENT_H
#define KEYSPACECLIENT_H

#include "System/Buffer.h"
#include "KeyspaceDB.h"

class KeyspaceOp;

class KeyspaceClient
{
public:
	virtual			~KeyspaceClient() {}
	virtual	void	OnComplete(KeyspaceOp* op, bool status) = 0;
	
	void Init(KeyspaceDB* kdb_)
	{
		kdb = kdb_;
		numpending = 0;
	}


	bool Add(KeyspaceOp &op)
	{
		bool ret;
		
		ret = kdb->Add(op);
		if (ret)
			numpending++;
		return ret;
	}

protected:
	int				numpending;
	KeyspaceDB*		kdb;
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
		DELETE,
		LIST
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
