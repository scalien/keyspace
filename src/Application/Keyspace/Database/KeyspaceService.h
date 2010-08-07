#ifndef KEYSPACESERVICE_H
#define KEYSPACESERVICE_H

#include "System/Buffer.h"
#include "KeyspaceDB.h"

class KeyspaceOp;

class KeyspaceService
{
public:
	virtual			~KeyspaceService() {}
	virtual	void	OnComplete(KeyspaceOp* op, bool final = true) = 0;
	virtual bool	IsAborted() = 0;

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
		COUNT,
		DIRTY_COUNT,
		SET,
		TEST_AND_SET,
		ADD,
		RENAME,
		DELETE,
		REMOVE,
		PRUNE,
		SET_EXPIRY,
		EXPIRE
	};
	
	bool					appended;
	
	Type					type;
	uint64_t				cmdID;
	ByteBuffer				key;
	ByteBuffer				newKey; // for rename
	ByteBuffer				value;
	ByteBuffer				test;
	ByteBuffer				prefix;
	int64_t					num;
	uint64_t				count;
	uint64_t				offset;
	uint64_t				expiryTime;
	bool					forward;
	bool					status;
	
	KeyspaceService*		service;
	
	KeyspaceOp()
	{
		appended = false;
		status = false;
	}
	
	~KeyspaceOp()
	{
		Free();
	}
	
	void Free()
	{
		key.Free();
		newKey.Free();
		value.Free();
		test.Free();
		prefix.Free();
	}
	
	bool IsAborted()
	{
		return service->IsAborted();
	}
	
	bool IsWrite()
	{
		return (type == KeyspaceOp::SET ||
				type == KeyspaceOp::TEST_AND_SET ||
				type == KeyspaceOp::DELETE ||
				type == KeyspaceOp::REMOVE ||
				type == KeyspaceOp::ADD ||
				type == KeyspaceOp::RENAME ||
				type == KeyspaceOp::PRUNE ||
				type == KeyspaceOp::SET_EXPIRY ||
				type == KeyspaceOp::EXPIRE);
	}

	bool IsRead()
	{
		return !IsWrite();
	}
	
	bool IsGet()
	{
		return (type == GET || type == DIRTY_GET);
	}
	
	bool IsListKeys()
	{
		return (type == LIST || type == DIRTY_LIST);
	}

	bool IsListKeyValues()
	{
		return (type == LISTP || type == DIRTY_LISTP);
	}
	
	bool IsList()
	{
		return (type == LIST ||
				type == DIRTY_LIST ||
				type == LISTP ||
				type == DIRTY_LISTP);
	}

	bool IsCount()
	{
		return (type == COUNT || type == DIRTY_COUNT);
	}
	
	bool IsDirty()
	{
		return (type == DIRTY_GET ||
				type == DIRTY_LIST ||
				type == DIRTY_LISTP ||
				type == DIRTY_COUNT
				);
	}
	
	bool MasterOnly()
	{
		return (type == GET ||
				type == LIST ||
				type == LISTP ||
				type == COUNT ||
				IsWrite());
	}
};

#endif
