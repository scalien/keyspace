#ifndef KEYSPACESERVICE_H
#define KEYSPACESERVICE_H

#include "System/Buffer.h"
#include "KeyspaceDB.h"

class KeyspaceOp;

class KeyspaceService
{
public:
	virtual			~KeyspaceService() {}
	virtual	void	OnComplete(KeyspaceOp* op, bool status, bool final = true) = 0;
	
	void Init(KeyspaceDB* kdb_)
	{
		kdb = kdb_;
		numpending = 0;
	}


	bool Add(KeyspaceOp *op)
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
		GET,
		DIRTY_GET,
		LIST,
		DIRTY_LIST,
		LISTP,
		DIRTY_LISTP,
		SET,
		TEST_AND_SET,
		ADD,
		DELETE,
		PRUNE
	};
	
	bool					appended;
	
	Type					type;
	ByteBuffer				key;
	ByteBuffer				value;
	ByteBuffer				test;
	ByteBuffer				prefix;
	uint64_t				cmdID;
	int64_t					num;
	uint64_t				count;
	
	KeyspaceService*		service;
	
	KeyspaceOp()
	{
		appended = false;
	}
	
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
			type == KeyspaceOp::DELETE || type == KeyspaceOp::ADD ||
			type == KeyspaceOp::PRUNE);
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
		return (type == LIST || type == DIRTY_LIST || type == LISTP || type == DIRTY_LISTP);
	}

	
	bool IsDirty()
	{
		return (type == DIRTY_GET || type == DIRTY_LIST || DIRTY_LISTP);
	}
	
	bool MasterOnly()
	{
		return (type == GET || type == LIST || type == LISTP || IsWrite());
	}
};

#endif
