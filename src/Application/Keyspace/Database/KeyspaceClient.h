#ifndef KEYSPACECLIENT_H
#define KEYSPACECLIENT_H

#include "System/Buffer.h"
#include "KeyspaceDB.h"

class KeyspaceOp;

class KeyspaceClient
{
public:
	virtual			~KeyspaceClient() {}
	virtual	void	OnComplete(KeyspaceOp* op, bool status, bool final = true) = 0;
	
	void Init(KeyspaceDB* kdb_)
	{
		kdb = kdb_;
		numpending = 0;
	}


	bool Add(KeyspaceOp *op, bool submit = true)
	{
		bool ret;
		
		ret = kdb->Add(op, submit);
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
		GET,
		DIRTY_GET,
		LIST,
		DIRTY_LIST,
		SET,
		TEST_AND_SET,
		ADD,
		DELETE
	};
	
	Type					type;
	ByteBuffer				key;
	ByteBuffer				value;
	ByteBuffer				test;
	ByteBuffer				prefix;
	uint64_t				cmdID;
	int64_t					num;
	uint64_t				count;
	
	KeyspaceClient*			client;
	
	~KeyspaceOp()
	{
		Free();
	}
	
	void Free()
	{
		key.Free();
		value.Free();
		test.Free();
		prefix.Free();
	}
	
	bool IsWrite()
	{
		return (type == KeyspaceOp::SET || type == KeyspaceOp::TEST_AND_SET ||
			type == KeyspaceOp::DELETE || type == KeyspaceOp::ADD);
	}

	bool IsRead()
	{
		return !IsWrite();
	}
	
	bool IsGet()
	{
		return (type == GET || type == DIRTY_GET);
	}
	
	bool IsList()
	{
		return (type == LIST || type == DIRTY_LIST);
	}

	
	bool IsDirty()
	{
		return (type == DIRTY_GET || type == DIRTY_LIST);
	}
};

#endif
