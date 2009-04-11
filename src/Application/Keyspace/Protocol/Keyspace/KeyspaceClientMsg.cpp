#include "KeyspaceClientMsg.h"
#include <stdio.h>
#include <inttypes.h>
#include "Application/Keyspace/Database/KeyspaceClient.h"

void KeyspaceClientReq::Init(char type_)
{
	type = type_;
}

bool KeyspaceClientReq::GetMaster()
{
	Init(KEYSPACECLIENT_GETMASTER);
	return true;
}

bool KeyspaceClientReq::Get(ByteString key_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_GET);	
	
	key.Reallocate(key_.length);
	key.Set(key_);
	return true;
}

bool KeyspaceClientReq::DirtyGet(ByteString key_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_DIRTYGET);	
	
	key.Reallocate(key_.length);
	key.Set(key_);
	return true;
}

bool KeyspaceClientReq::List(ByteString prefix_, uint64_t count_)
{
	if (prefix_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_LIST);
	
	key.Reallocate(prefix_.length);
	key.Set(prefix_);
	count = count_;
	return true;
}

bool KeyspaceClientReq::DirtyList(ByteString prefix_, uint64_t count_)
{
	if (prefix_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_DIRTYLIST);
	
	key.Reallocate(prefix_.length);
	key.Set(prefix_);
	count = count_;
	return true;
}

bool KeyspaceClientReq::Set(ByteString key_, ByteString value_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;
	if (value_.length > KEYSPACE_VAL_SIZE)
		return false;

	Init(KEYSPACECLIENT_SET);
	
	key.Reallocate(key_.length);
	key.Set(key_);
	value.Reallocate(value_.length);
	value.Set(value_);
	return true;
}
	
bool KeyspaceClientReq::TestAndSet(ByteString key_, ByteString test_, ByteString value_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;
	if (test_.length > KEYSPACE_VAL_SIZE)
		return false;
	if (value_.length > KEYSPACE_VAL_SIZE)
		return false;

	Init(KEYSPACECLIENT_TESTANDSET);
	
	key.Reallocate(key_.length);
	key.Set(key_);
	test.Reallocate(test_.length);
	test.Set(test_);
	value.Reallocate(value_.length);
	value.Set(value_);
	return true;
}

bool KeyspaceClientReq::Add(ByteString key_, int64_t num_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_ADD);
	
	key.Reallocate(key_.length);
	key.Set(key_);
	num = num_;
	return true;
}

bool KeyspaceClientReq::Delete(ByteString key_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_DELETE);
	
	key.Reallocate(key_.length);
	key.Set(key_);
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
		
#define CheckOverflow()		if ((pos - data.buffer) >= data.length) return false;
#define ReadUint64_t(num)	(num) = strntouint64_t(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadInt64_t(num)	(num) = strntoint64_t(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadChar(c)			(c) = *pos; pos++;
#define ReadSeparator()		if (*pos != ':') return false; pos++;
#define ValidateLength()	if ((pos - data.buffer) != data.length) return false;

	pos = data.buffer;
	CheckOverflow();
	ReadChar(type);
	
	if (type == KEYSPACECLIENT_SUBMIT)
	{
		if (pos > data.buffer + data.length)
			return false;

		ValidateLength();
		Submit();
		return true;
	}
	else if (type == KEYSPACECLIENT_GETMASTER)
	{
		if (pos > data.buffer + data.length)
			return false;

		ValidateLength();
		GetMaster();
		return true;
	}
	
	CheckOverflow();
	ReadSeparator(); CheckOverflow();
	
	if (type == KEYSPACECLIENT_GET || type == KEYSPACECLIENT_DIRTYGET)
	{
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		
		ValidateLength();		
		if (type == KEYSPACECLIENT_GET)
			Get(ByteString(key.length, key.length, key.buffer));
		else
			DirtyGet(ByteString(key.length, key.length, key.buffer));
		return true;
	}
	else if (type == KEYSPACECLIENT_LIST || type == KEYSPACECLIENT_DIRTYLIST)
	{
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64_t(count);
		
		ValidateLength();
		if (type == KEYSPACECLIENT_LIST)
			List(ByteString(key.length, key.length, key.buffer), count);
		else
			DirtyList(ByteString(key.length, key.length, key.buffer), count);
		return true;
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
		Set(ByteString(key.length, key.length, key.buffer),
			ByteString(value.length, value.length, value.buffer));
		return true;
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
		TestAndSet(ByteString(key.length, key.length, key.buffer),
				   ByteString(test.length, test.length, test.buffer),
				   ByteString(value.length, value.length, value.buffer));
		return true;
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
		Add(ByteString(key.length, key.length, key.buffer), num);
		return true;
	}
	else if (type == KEYSPACECLIENT_DELETE)
	{
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		
		ValidateLength();
		Delete(ByteString(key.length, key.length, key.buffer));
		return true;
	}
	
	return false;
}

bool KeyspaceClientReq::Write(ByteString& data)
{
	int required;
	
	if (type == KEYSPACECLIENT_SUBMIT || type == KEYSPACECLIENT_GETMASTER)
			required = snprintf(data.buffer, data.size, "%c", type);
	else if (type == KEYSPACECLIENT_GET || type == KEYSPACECLIENT_DIRTYGET)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s", type,
			key.length, key.length, key.buffer);
	else if (type == KEYSPACECLIENT_LIST || type == KEYSPACECLIENT_DIRTYLIST)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s:%" PRIu64 "", type,
			key.length, key.length, key.buffer, count);
	else if (type == KEYSPACECLIENT_SET)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s:%d:%.*s", type,
			key.length, key.length, key.buffer,
			value.length, value.length, value.buffer);
	else if (type == KEYSPACECLIENT_TESTANDSET)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s:%d:%.*s:%d:%.*s", type,
			key.length, key.length, key.buffer,
			test.length, test.length, test.buffer,
			value.length, value.length, value.buffer);
	else if (type == KEYSPACECLIENT_ADD)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s:%" PRIi64 "", type,
			key.length, key.length, key.buffer, num);
	else if (type == KEYSPACECLIENT_DELETE)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s", type,
			key.length, key.length, key.buffer);
	else
		return false;
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;
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
	else if (type == KEYSPACECLIENT_SET)
		op->type = KeyspaceOp::SET;
	else if (type == KEYSPACECLIENT_TESTANDSET)
		op->type = KeyspaceOp::TEST_AND_SET;
	else if (type == KEYSPACECLIENT_DELETE)
		op->type = KeyspaceOp::DELETE;
	else if (type == KEYSPACECLIENT_ADD)
		op->type = KeyspaceOp::ADD;
	
	if (type == KEYSPACECLIENT_LIST || type == KEYSPACECLIENT_DIRTYLIST)
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



void KeyspaceClientResp::Ok()
{
	type = KEYSPACECLIENT_OK;
	value.length = 0;
}

void KeyspaceClientResp::Ok(ByteString value_)
{
	type = KEYSPACECLIENT_OK;
	value = value_;
}

void KeyspaceClientResp::NotFound()
{
	type = KEYSPACECLIENT_NOTFOUND;
	value.length = 0;
}

void KeyspaceClientResp::Failed()
{
	type = KEYSPACECLIENT_FAILED;
	value.length = 0;
}

void KeyspaceClientResp::ListItem(ByteString value_)
{
	type = KEYSPACECLIENT_LISTITEM;
	value = value_;
}

void KeyspaceClientResp::ListEnd()
{
	type = KEYSPACECLIENT_LISTEND;
	value.length = 0;
}

bool KeyspaceClientResp::Write(ByteString& data)
{
	int required;
	
	if (type == KEYSPACECLIENT_LISTITEM)
		required = snprintf(data.buffer, data.size, "%d:%.*s",
			value.length, value.length, value.buffer);
	else if (value.length > 0)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s",
			type, value.length, value.length, value.buffer);
	else
		required = snprintf(data.buffer, data.size, "%c", type);
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;
}
