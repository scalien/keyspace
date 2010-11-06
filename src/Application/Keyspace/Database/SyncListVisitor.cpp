#include "SyncListVisitor.h"
#include "System/IO/IOProcessor.h"
#include "KeyspaceDB.h"

SyncListVisitor::SyncListVisitor(KeyspaceOp* op_) :
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
    
    completed = true;

	num = 0;
}

void SyncListVisitor::AppendKey(const ByteString &key)
{
    op->key.Set(key);
	op->status = true;
    op->service->OnComplete(op, false);
}

void SyncListVisitor::AppendKeyValue(const ByteString &key,
									  const ByteString &value)
{
    ByteString      userValue;
    uint64_t        storedPaxosID;
    uint64_t        storedCommandID;
    
    KeyspaceDB::ReadValue(value, storedPaxosID,
     storedCommandID, userValue);

    op->key.Set(key);
    op->value.Set(userValue);
	op->status = true;
    op->service->OnComplete(op, false);
}

// this is called by MultiDatabaseOp::Visit in the 
// dbReader thread pool
bool SyncListVisitor::Accept(const ByteString &key,
							  const ByteString &value)
{
	if (op->IsAborted())
    {
        completed = true;
		return false;
    }
    
    if (num > 0 && num % LIST_RUN_GRANULARITY == 0)
    {
        completed = false;
        return false;
    }
    
	// don't list system keys
	if (key.length >= 2 && key.buffer[0] == '@' && key.buffer[1] == '@')
		return true;
	if (key.length >= 2 && key.buffer[0] == '!' && key.buffer[1] == '!')
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
			num++;
		}
		return true;
	}
	else
    {
        completed = true;
		return false;
    }

}

const ByteString* SyncListVisitor::GetStartKey()
{
	return &startKey;
}

void SyncListVisitor::OnComplete()
{
}

bool SyncListVisitor::IsForward()
{
	return op->forward;
}
