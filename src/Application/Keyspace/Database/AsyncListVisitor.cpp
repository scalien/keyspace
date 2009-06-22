#include "AsyncListVisitor.h"
#include "System/IO/IOProcessor.h"

AsyncVisitorCallback::AsyncVisitorCallback()
{
	Log_Trace();

	numkey = 0;
	op = NULL;
}

AsyncVisitorCallback::~AsyncVisitorCallback()
{
}

bool AsyncVisitorCallback::AppendKey(const ByteString &key)
{
	Log_Trace();
	
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

		valuepos[numvalue] = valuebuf.length;
		valuelen[numvalue] = value.length;
		valuebuf.Append(value.buffer, value.length);
		
		return true;
	}
	
	return false;
}

void AsyncVisitorCallback::Execute()
{
	Log_Trace();

	op->status = true;
	
	for (int i = 0; i < numkey; i++)
	{
		// HACK in order to not copy the buffer we set the members of
		// the key and value manually and set it back before deleting 
		op->key.buffer = keys[i].buffer;
		op->key.size = keys[i].size;
		op->key.length = keys[i].length;
		
		if (op->type == KeyspaceOp::LISTP || op->type == KeyspaceOp::DIRTY_LISTP)
		{
			// this is a huge hack, since op->value is a ByteBuffer!
			// if it were allocated, this would result in memleak
			op->value.buffer = valuebuf.buffer + valuepos[i];
			op->value.size = valuelen[i];
			op->value.length = valuelen[i];
		}
		
		op->service->OnComplete(op, false);
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
	{
		Log_Trace("calling with complete==true");
		op->service->OnComplete(op, complete);
	}
	
	delete this;
}

AsyncListVisitor::AsyncListVisitor(KeyspaceOp* op_) :
prefix(op_->prefix)
{
	op = op_;
	count = op->count;
	offset = op->offset;
	startKey.Append(prefix);
	startKey.Append(op->key);
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
	Log_Trace();

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
	Log_Trace();

	// don't list system keys
	if (key.length >= 2 && key.buffer[0] == '@' && key.buffer[1] == '@')
		return true;
	
	if (num == 0 && startKey != key && offset > 0)
		offset--;

	if ((count == 0 || num < count) &&
		(prefix.length == 0 ||
		strncmp(prefix.buffer, key.buffer, min(prefix.length, key.length)) == 0))
	{
		if (offset > 0)
			offset--;
		else
		{
			if (op->type == KeyspaceOp::LIST || op->type == KeyspaceOp::DIRTY_LIST)
				AppendKey(key);
			if (op->type == KeyspaceOp::LISTP || op->type == KeyspaceOp::DIRTY_LISTP)
				AppendKeyValue(key, value);
			num++;
		}
		return true;
	}
	else
		return false;

}

const ByteString* AsyncListVisitor::GetStartKey()
{
	return &startKey;
}

void AsyncListVisitor::OnComplete()
{
	Log_Trace();

	Log_Trace("setting complete=true");
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
