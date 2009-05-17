#include "AsyncListVisitor.h"
#include "IOProcessor.h"

AsyncVisitorCallback::AsyncVisitorCallback()
{
	numkey = 0;
	op = NULL;
}

AsyncVisitorCallback::~AsyncVisitorCallback()
{
}

bool AsyncVisitorCallback::AppendKey(const ByteString &key)
{
	if (key.length + keybuf.length <= keybuf.size && numkey < VISITOR_LIMIT - 1)
	{
		keys[numkey].length = key.length;
		keys[numkey].size = key.size;
		keys[numkey].buffer = keybuf.buffer + keybuf.length;
		memcpy(keybuf.buffer + keybuf.length, key.buffer, key.length);
		keybuf.length += key.length;
		
		numkey++;
		return true;
	}
	
	return false;
}

bool AsyncVisitorCallback::AppendKeyValue(const ByteString &key, const ByteString &value)
{
	int numvalue;

	if (key.length + keybuf.length <= keybuf.size && 
		numkey < VISITOR_LIMIT - 1 &&
		value.length + valuebuf.length <= valuebuf.size)
	{
		// save numkey here, because AppendKey increments it
		numvalue = numkey;
		AppendKey(key);

		values[numvalue].length = value.length;
		values[numvalue].size = value.size;
		values[numvalue].buffer = valuebuf.buffer + valuebuf.length;
		valuebuf.Append(value.buffer, value.length);
		
		return true;
	}
	
	return false;
}

void AsyncVisitorCallback::Execute()
{
	for (int i = 0; i < numkey; i++)
	{
		// HACK in order to not copy the buffer we set the members of
		// the key and value manually and set it back before deleting 
		op->key.buffer = keys[i].buffer;
		op->key.size = keys[i].size;
		op->key.length = keys[i].length;
		
		if (op->type == KeyspaceOp::LISTP || op->type == KeyspaceOp::DIRTY_LISTP)
		{
			op->value.buffer = values[i].buffer;
			op->value.size = values[i].size;
			op->value.length = values[i].length;
		}
		
		op->service->OnComplete(op, true, false);
	}

	op->key.buffer = NULL;
	op->key.size = 0;
	op->key.length = 0;
	
	if (op->type == KeyspaceOp::LISTP || op->type == KeyspaceOp::DIRTY_LISTP)
	{
		op->value.buffer = NULL;
		op->value.size = 0;
		op->value.length = 0;
	}

	if (complete)
		op->service->OnComplete(op, true, complete);

	delete this;
}

AsyncListVisitor::AsyncListVisitor(const ByteString &keyHint_, KeyspaceOp* op_, uint64_t count_) :
keyHint(keyHint_)
{
	op = op_;
	count = count_;
	num = 0;
	Init();
}

void AsyncListVisitor::Init()
{
	avc = new AsyncVisitorCallback;
	avc->op = op;
}

void AsyncListVisitor::AppendKey(const ByteString &key)
{
	if (!avc->AppendKey(key))
	{
		IOProcessor::Complete(avc);
		Init();
		avc->AppendKey(key);
	}
}

void AsyncListVisitor::AppendKeyValue(const ByteString &key, const ByteString &value)
{
	if (!avc->AppendKeyValue(key, value))
	{
		IOProcessor::Complete(avc);
		Init();
		avc->AppendKeyValue(key, value);
	}
}

bool AsyncListVisitor::Accept(const ByteString &key, const ByteString &value)
{
	// don't list system keys
	if (key.length >= 2 && key.buffer[0] == '@' && key.buffer[1] == '@')
		return true;

	if ((count == 0 || num < count) &&
		strncmp(keyHint.buffer, key.buffer, min(keyHint.length, key.length)) == 0)
	{
		if (op->type == KeyspaceOp::LIST || op->type == KeyspaceOp::DIRTY_LIST)
			AppendKey(key);
		if (op->type == KeyspaceOp::LISTP || op->type == KeyspaceOp::DIRTY_LISTP)
			AppendKeyValue(key, value);
		num++;
		return true;
	}
	else
		return false;
}

const ByteString* AsyncListVisitor::GetKeyHint()
{
	return &keyHint;
}

void AsyncListVisitor::OnComplete()
{
	avc->complete = true;
	IOProcessor::Complete(avc);
	delete this;
}

AsyncMultiDatabaseOp::AsyncMultiDatabaseOp()
{
	SetCallback(this);
}

void AsyncMultiDatabaseOp::Execute()
{
	Log_Trace();
	delete this;
}
