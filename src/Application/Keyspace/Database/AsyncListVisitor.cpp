#include "AsyncListVisitor.h"
#include "System/IO/IOProcessor.h"

AsyncVisitorCallback::AsyncVisitorCallback()
{
	Log_Trace();

	numkey = 0;
	op = NULL;
	complete = false;
}

AsyncVisitorCallback::~AsyncVisitorCallback()
{
}

void AsyncVisitorCallback::Increment()
{
	numkey++;
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

bool AsyncVisitorCallback::AppendKeyValue(const ByteString &key,
const ByteString &value)
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

// this is the callback running in the main thread
void AsyncVisitorCallback::Execute()
{
	Log_Trace();

	op->status = true;
	
	if (op->IsCount())
	{
		op->value.Allocate(CS_INT_SIZE(uint64_t));
		op->value.length = snwritef(op->value.buffer, op->value.size,
									"%I", numkey);
	}
	else
	{
		for (uint64_t i = 0; i < numkey; i++)
		{
			// HACK in order to not copy the buffer we set the members of
			// the key and value manually and set it back before deleting 
			op->key.buffer = keys[i].buffer;
			op->key.size = keys[i].size;
			op->key.length = keys[i].length;
			
			if (op->IsListKeyValues())
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
		
		if (op->IsListKeyValues())
		{
			op->value.buffer = NULL;
			op->value.size = 0;
			op->value.length = 0;
		}
	}

	if (complete)
	{
		Log_Trace("calling with complete==true");
		op->service->OnComplete(op, complete);
	}
	
	// as AsyncVisitorCallback is allocated both in the main
	// thread and also in the thread pool, it is responsible
	// for deleting itself.
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

	// TODO: as in AsyncVisitorCallback we reuse keys and values we need to Free them here
	op->key.Free();
	op->value.Free();

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

void AsyncListVisitor::AppendKeyValue(const ByteString &key,
									  const ByteString &value)
{
	if (!avc->AppendKeyValue(key, value))
	{
		IOProcessor::Complete(avc);
		Init();
		avc->AppendKeyValue(key, value);
	}
}

// this is called by MultiDatabaseOp::Visit in the 
// dbReader thread pool
bool AsyncListVisitor::Accept(const ByteString &key,
							  const ByteString &value)
{
	if (op->IsAborted())
		return false;

	// don't list system keys
	if (key.length >= 2 && key.buffer[0] == '@' && key.buffer[1] == '@')
		return true;
	
	if (num == 0 && startKey != key && offset > 0)
	{
		offset--;
		return true;
	}

	if ((count == 0 || num < count) &&
		(prefix.length == 0 ||
		(key.length >= prefix.length &&
		strncmp(prefix.buffer, key.buffer,
		MIN(prefix.length, key.length)) == 0)))
	{
		if (offset > 0)
			offset--;
		else
		{
			if (op->IsListKeys())
				AppendKey(key);
			else if (op->IsListKeyValues())
				AppendKeyValue(key, value);
			else if (op->IsCount())
				avc->Increment();
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

bool AsyncListVisitor::IsForward()
{
	return op->forward;
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
