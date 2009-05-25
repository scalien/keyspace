#include "KeyspaceClientMsg.h"
#include <stdio.h>
#include <inttypes.h>
#include "Application/Keyspace/Database/KeyspaceService.h"

void KeyspaceClientReq::Init(char type_)
{
	type = type_;
}

bool KeyspaceClientReq::GetMaster(uint64_t cmdID_)
{
	Init(KEYSPACECLIENT_GETMASTER);
	cmdID = cmdID_;
	return true;
}

bool KeyspaceClientReq::Get(uint64_t cmdID_, ByteString key_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_GET);
	cmdID = cmdID_;
	
	key.Reallocate(key_.length);
	key.Set(key_);
	return true;
}

bool KeyspaceClientReq::DirtyGet(uint64_t cmdID_, ByteString key_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_DIRTYGET);	
	cmdID = cmdID_;
	
	key.Reallocate(key_.length);
	key.Set(key_);
	return true;
}

bool KeyspaceClientReq::List(uint64_t cmdID_, ByteString prefix_, uint64_t count_)
{
	if (prefix_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_LIST);
	cmdID = cmdID_;
	
	key.Reallocate(prefix_.length);
	key.Set(prefix_);
	count = count_;
	return true;
}

bool KeyspaceClientReq::DirtyList(uint64_t cmdID_, ByteString prefix_, uint64_t count_)
{
	if (prefix_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_DIRTYLIST);
	cmdID = cmdID_;
	
	key.Reallocate(prefix_.length);
	key.Set(prefix_);
	count = count_;
	return true;
}

bool KeyspaceClientReq::ListP(uint64_t cmdID_, ByteString prefix_, uint64_t count_)
{
	if (prefix_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_LISTP);
	cmdID = cmdID_;
	
	key.Reallocate(prefix_.length);
	key.Set(prefix_);
	count = count_;
	return true;
}

bool KeyspaceClientReq::DirtyListP(uint64_t cmdID_, ByteString prefix_, uint64_t count_)
{
	if (prefix_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_DIRTYLISTP);
	cmdID = cmdID_;
	
	key.Reallocate(prefix_.length);
	key.Set(prefix_);
	count = count_;
	return true;
}

bool KeyspaceClientReq::Set(uint64_t cmdID_, ByteString key_, ByteString value_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;
	if (value_.length > KEYSPACE_VAL_SIZE)
		return false;

	Init(KEYSPACECLIENT_SET);
	cmdID = cmdID_;
	
	key.Reallocate(key_.length);
	key.Set(key_);
	value.Reallocate(value_.length);
	value.Set(value_);
	return true;
}
	
bool KeyspaceClientReq::TestAndSet(uint64_t cmdID_, ByteString key_, ByteString test_, ByteString value_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;
	if (test_.length > KEYSPACE_VAL_SIZE)
		return false;
	if (value_.length > KEYSPACE_VAL_SIZE)
		return false;

	Init(KEYSPACECLIENT_TESTANDSET);
	cmdID = cmdID_;
	
	key.Reallocate(key_.length);
	key.Set(key_);
	test.Reallocate(test_.length);
	test.Set(test_);
	value.Reallocate(value_.length);
	value.Set(value_);
	return true;
}

bool KeyspaceClientReq::Add(uint64_t cmdID_, ByteString key_, int64_t num_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_ADD);
	cmdID = cmdID_;
	
	key.Reallocate(key_.length);
	key.Set(key_);
	num = num_;
	return true;
}

bool KeyspaceClientReq::Delete(uint64_t cmdID_, ByteString key_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_DELETE);
	cmdID = cmdID_;
	
	key.Reallocate(key_.length);
	key.Set(key_);
	return true;
}

bool KeyspaceClientReq::Prune(uint64_t cmdID_, ByteString prefix_)
{
	if (prefix_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_PRUNE);
	cmdID = cmdID_;
	
	if (prefix_.length > 0)
	{
		key.Reallocate(prefix_.length);
		key.Set(prefix_);
	}
	else
		key.length = 0;

	return true;
}

bool KeyspaceClientReq::Submit()
{
	Init(KEYSPACECLIENT_SUBMIT);
	return true;
}
	
bool KeyspaceClientReq::Read(ByteString data)
{
	unsigned	nread;
	char		*pos;
	ByteString  key, value, test;
		
#define CheckOverflow()		if ((pos - data.buffer) >= (int) data.length || pos < data.buffer) return false;
#define ReadUint64_t(num)	(num) = strntouint64_t(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadInt64_t(num)	(num) = strntoint64_t(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadChar(c)			(c) = *pos; pos++;
#define ReadSeparator()		if (*pos != ':') return false; pos++;
#define ValidateLength()	if ((pos - data.buffer) != (int) data.length) return false;

	pos = data.buffer;
	CheckOverflow();
	ReadChar(type);
	
	if (type == KEYSPACECLIENT_SUBMIT)
	{
		if (pos > data.buffer + data.length)
			return false;

		ValidateLength();
		return Submit();
	}
	
	CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadUint64_t(cmdID);
		
	if (type == KEYSPACECLIENT_GETMASTER)
	{
		if (pos > data.buffer + data.length)
			return false;

		ValidateLength();
		return GetMaster(cmdID);
	}
	
	CheckOverflow();
	ReadSeparator();
	
	if (type == KEYSPACECLIENT_GET || type == KEYSPACECLIENT_DIRTYGET)
	{
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		
		ValidateLength();		
		if (type == KEYSPACECLIENT_GET)
			return Get(cmdID, ByteString(key.length, key.length, key.buffer));
		else
			return DirtyGet(cmdID, ByteString(key.length, key.length, key.buffer));
	}
	else if (type == KEYSPACECLIENT_LIST || type == KEYSPACECLIENT_DIRTYLIST || 
			 type == KEYSPACECLIENT_LISTP || type == KEYSPACECLIENT_DIRTYLISTP)
	{
		unsigned countlen;
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64_t(countlen); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadInt64_t(count);
		
		ValidateLength();
		if (type == KEYSPACECLIENT_LIST)
			return List(cmdID, ByteString(key.length, key.length, key.buffer), count);
		else if (type == KEYSPACECLIENT_DIRTYLIST)
			return DirtyList(cmdID, ByteString(key.length, key.length, key.buffer), count);
		else if (type == KEYSPACECLIENT_LISTP)
			return ListP(cmdID, ByteString(key.length, key.length, key.buffer), count);
		else if (type == KEYSPACECLIENT_DIRTYLISTP)
			return DirtyListP(cmdID, ByteString(key.length, key.length, key.buffer), count);

	}
	else if (type == KEYSPACECLIENT_SET)
	{
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64_t(value.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		value.buffer = pos;
		pos += value.length;
		
		ValidateLength();
		return Set(cmdID, ByteString(key.length, key.length, key.buffer),
				   ByteString(value.length, value.length, value.buffer));
	}
	else if (type == KEYSPACECLIENT_TESTANDSET)
	{
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64_t(test.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		test.buffer = pos;
		pos += test.length;
		
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64_t(value.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		value.buffer = pos;
		pos += value.length;
		
		ValidateLength();
		return TestAndSet(cmdID, ByteString(key.length, key.length, key.buffer),
						  ByteString(test.length, test.length, test.buffer),
						  ByteString(value.length, value.length, value.buffer));
	}
	else if (type == KEYSPACECLIENT_ADD)
	{
		unsigned numlen;
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64_t(numlen); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadInt64_t(num);
		
		ValidateLength();
		return Add(cmdID, ByteString(key.length, key.length, key.buffer), num);
	}
	else if (type == KEYSPACECLIENT_DELETE)
	{
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		
		ValidateLength();
		return Delete(cmdID, ByteString(key.length, key.length, key.buffer));
	}
	else if (type == KEYSPACECLIENT_PRUNE)
	{
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); if (key.length > 0) CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		
		ValidateLength();
		return Prune(cmdID, ByteString(key.length, key.length, key.buffer));
	}
	
	return false;
}

bool KeyspaceClientReq::ToKeyspaceOp(KeyspaceOp* op)
{
	if (type == KEYSPACECLIENT_GET)
		op->type = KeyspaceOp::GET;
	else if (type == KEYSPACECLIENT_DIRTYGET)
		op->type = KeyspaceOp::DIRTY_GET;
	else if (type == KEYSPACECLIENT_LIST)
		op->type = KeyspaceOp::LIST;
	else if (type == KEYSPACECLIENT_DIRTYLIST)
		op->type = KeyspaceOp::DIRTY_LIST;
	else if (type == KEYSPACECLIENT_LISTP)
		op->type = KeyspaceOp::LISTP;
	else if (type == KEYSPACECLIENT_DIRTYLISTP)
		op->type = KeyspaceOp::DIRTY_LISTP;
	else if (type == KEYSPACECLIENT_SET)
		op->type = KeyspaceOp::SET;
	else if (type == KEYSPACECLIENT_TESTANDSET)
		op->type = KeyspaceOp::TEST_AND_SET;
	else if (type == KEYSPACECLIENT_DELETE)
		op->type = KeyspaceOp::DELETE;
	else if (type == KEYSPACECLIENT_PRUNE)
		op->type = KeyspaceOp::PRUNE;
	else if (type == KEYSPACECLIENT_ADD)
		op->type = KeyspaceOp::ADD;
	else
		ASSERT_FAIL();
	
	op->cmdID = cmdID;
	
	if (type == KEYSPACECLIENT_PRUNE)
	{
		if (key.length == 0)
			op->prefix.length = 0;
		else
		{
			if (!op->prefix.Reallocate(key.length))
				return false;
			op->prefix.Set(key);
		}
	}
	else if (type == KEYSPACECLIENT_LIST || type == KEYSPACECLIENT_DIRTYLIST ||
		type == KEYSPACECLIENT_LISTP || type == KEYSPACECLIENT_DIRTYLISTP)
	{
		if (!op->prefix.Reallocate(key.length))
			return false;
		op->prefix.Set(key);
		op->count = count;
	}
	else
	{
		if (!op->key.Reallocate(key.length))
			return false;
		op->key.Set(key);
	}
	if (type == KEYSPACECLIENT_SET || type == KEYSPACECLIENT_TESTANDSET)
	{
		if (!op->value.Reallocate(value.length))
			return false;
		op->value.Set(value);
	}
	if (type == KEYSPACECLIENT_TESTANDSET)
	{
		if (!op->test.Reallocate(test.length))
			return false;
		op->test.Set(test);
	}
	if (type == KEYSPACECLIENT_ADD)
		op->num = num;
		
	return true;
}


void KeyspaceClientResp::Ok(uint64_t cmdID_)
{
	type = KEYSPACECLIENT_OK;
	cmdID = cmdID_;
	key.length = 0;
	value.length = 0;
}

void KeyspaceClientResp::Ok(uint64_t cmdID_, ByteString value_)
{
	type = KEYSPACECLIENT_OK;
	cmdID = cmdID_;
	key.length = 0;
	value = value_;
}

void KeyspaceClientResp::Failed(uint64_t cmdID_)
{
	type = KEYSPACECLIENT_FAILED;
	cmdID = cmdID_;
	key.length = 0;
	value.length = 0;
}

void KeyspaceClientResp::NotMaster(uint64_t cmdID_)
{
	type = KEYSPACECLIENT_NOTMASTER;
	cmdID = cmdID_;
	key.length = 0;
	value.length = 0;
}

void KeyspaceClientResp::ListItem(uint64_t cmdID_, ByteString key_)
{
	type = KEYSPACECLIENT_LISTITEM;
	cmdID = cmdID_;
	key = key_;
	value.length = 0;
}

void KeyspaceClientResp::ListPItem(uint64_t cmdID_, ByteString key_, ByteString value_)
{
	type = KEYSPACECLIENT_LISTPITEM;
	cmdID = cmdID_;
	key = key_;
	value = value_;
}

void KeyspaceClientResp::ListEnd(uint64_t cmdID_)
{
	type = KEYSPACECLIENT_LISTEND;
	cmdID = cmdID_;
	key.length = 0;
	value.length = 0;
}

bool KeyspaceClientResp::Write(ByteString& data)
{
	int required;
	
	if (key.length > 0 && value.length > 0)
		required = snwritef(data.buffer, data.size, "%c:%U:%M:%M",
			type, cmdID, key.length, key.buffer, value.length, value.buffer);
	else if (key.length > 0)
		required = snwritef(data.buffer, data.size, "%c:%U:%M",
			type, cmdID, key.length, key.buffer);
	else if (value.length > 0)
		required = snwritef(data.buffer, data.size, "%c:%U:%M",
			type, cmdID, value.length, value.buffer);
	else
		required = snwritef(data.buffer, data.size, "%c:%U", type, cmdID);
	
	if (required < 0 || (unsigned)required > data.size)
		return false;
		
	data.length = required;
	return true;
}
