#ifndef SYNCLISTVISITOR_H
#define SYNCLISTVISITOR_H

#include "System/Events/Callable.h"
#include "System/Buffer.h"
#include "Framework/Database/Table.h"
#include "Framework/AsyncDatabase/MultiDatabaseOp.h"
#include "KeyspaceService.h"

#define LIST_RUN_GRANULARITY    1000

class SyncListVisitor : public TableVisitor
{
public:
	SyncListVisitor(KeyspaceOp* op_);
	
	void						AppendKey(const ByteString &key);
	void						AppendKeyValue(const ByteString &key,
											   const ByteString &value);
	virtual bool				Accept(const ByteString &key,
									   const ByteString &value);
	virtual const ByteString*	GetStartKey();
	virtual void				OnComplete();
	virtual bool				IsForward();
	
    bool                        completed;
	KeyspaceOp*					op;
	const ByteString			&prefix;
	DynArray<128>				startKey;
	uint64_t					count;
	uint64_t					offset;
	uint64_t					num;
};

#endif
