#ifndef ASYNCLISTVISITOR_H
#define ASYNCLISTVISITOR_H

#include "System/Events/Callable.h"
#include "System/Buffer.h"
#include "Framework/Database/Table.h"
#include "Framework/AsyncDatabase/MultiDatabaseOp.h"
#include "KeyspaceService.h"

#define VISITOR_LIMIT	4096

class AsyncVisitorCallback : public Callable
{
public:
	typedef ByteArray<VISITOR_LIMIT>		KeyBuffer; // TODO better initial value
	typedef DynArray<100 * VISITOR_LIMIT>	ValueBuffer;
	
	ByteString		keys[VISITOR_LIMIT];
	ByteString		values[VISITOR_LIMIT];
	KeyBuffer		keybuf;
	ValueBuffer		valuebuf;
	int				numkey;
	KeyspaceOp*		op;
	bool			complete;
	
	AsyncVisitorCallback();
	~AsyncVisitorCallback();

	bool AppendKey(const ByteString &key);
	bool AppendKeyValue(const ByteString &key, const ByteString &value);
	void Execute();
};

class AsyncListVisitor : public TableVisitor
{
public:
	AsyncListVisitor(KeyspaceOp* op_);
	
	void Init();
	void AppendKey(const ByteString &key);
	void AppendKeyValue(const ByteString &key, const ByteString &value);
	virtual bool Accept(const ByteString &key, const ByteString &value);
	virtual const ByteString* GetStartKey();
	virtual void OnComplete();
	
private:
	KeyspaceOp*				op;
	const ByteString		&prefix;
	const ByteString		&startKey;
	uint64_t				count;
	uint64_t				offset;
	uint64_t				num;
	AsyncVisitorCallback*	avc;
};

class AsyncMultiDatabaseOp : public Callable, public MultiDatabaseOp
{
public:
	AsyncMultiDatabaseOp();
	
	void Execute();
};

#endif
