#include "KeyspaceMsg.h"
#include "KeyspaceClient.h"
#include <stdio.h>

void KeyspaceMsg::Init(char type_)
{
	type = type_;
}

void KeyspaceMsg::Get(ByteString key_)
{
	Init(KEYSPACE_GET);
	
	key.Set(key_);
}
	
void KeyspaceMsg::Set(ByteString key_, ByteString value_)
{
	Init(KEYSPACE_SET);
	
	key.Set(key_);
	value.Set(value_);
}
	
void KeyspaceMsg::TestAndSet(ByteString key_, ByteString test_, ByteString value_)
{
	Init(KEYSPACE_TESTANDSET);
	
	key.Set(key_);
	test.Set(test_);
	value.Set(value_);
}

void KeyspaceMsg::Add(ByteString key_, int64_t num_)
{
	Init(KEYSPACE_ADD);
	
	key.Set(key_);
	num = num_;
}

void KeyspaceMsg::Delete(ByteString key_)
{
	Init(KEYSPACE_DELETE);
	
	key.Set(key_);
}
	
bool KeyspaceMsg::Read(ByteString& data, unsigned &n)
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
	ReadChar(type); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	
	if (type == KEYSPACE_GET)
	{
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		
		if (pos > data.buffer + data.length)
			return false;
		
		Get(ByteString(key.length, key.length, key.buffer));
		n = pos - data.buffer;
		return true;
	}
	else if (type == KEYSPACE_SET)
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
		
		if (pos > data.buffer + data.length)
			return false;
		
		Set(ByteString(key.length, key.length, key.buffer),
			ByteString(value.length, value.length, value.buffer));
		n = pos - data.buffer;
		return true;
	}
	else if (type == KEYSPACE_TESTANDSET)
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
		
		if (pos > data.buffer + data.length)
			return false;
		
		TestAndSet(ByteString(key.length, key.length, key.buffer),
				   ByteString(test.length, test.length, test.buffer),
				   ByteString(value.length, value.length, value.buffer));
		n = pos - data.buffer;
		return true;
	}
	else if (type == KEYSPACE_ADD)
	{
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadInt64_t(num);
		
		if (pos > data.buffer + data.length)
			return false;
		
		Add(ByteString(key.length, key.length, key.buffer), num);
		n = pos - data.buffer;
		return true;
	}
	else if (type == KEYSPACE_DELETE)
	{
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		
		if (pos > data.buffer + data.length)
			return false;
		
		Delete(ByteString(key.length, key.length, key.buffer));
		n = pos - data.buffer;
		return true;
	}
	
	return false;
}

bool KeyspaceMsg::Write(ByteString& data)
{
	int required;
	
	if (type == KEYSPACE_GET)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s", type,
			key.length, key.length, key.buffer);
	else if (type == KEYSPACE_SET)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s:%d:%.*s", type,
			key.length, key.length, key.buffer,
			value.length, value.length, value.buffer);
	else if (type == KEYSPACE_TESTANDSET)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s:%d:%.*s:%d:%.*s", type,
			key.length, key.length, key.buffer,
			test.length, test.length, test.buffer,
			value.length, value.length, value.buffer);
	else if (type == KEYSPACE_ADD)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s:%" PRIi64 "", type,
			key.length, key.length, key.buffer, num);
	else if (type == KEYSPACE_DELETE)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s", type,
			key.length, key.length, key.buffer);
	else
		return false;
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;

}

bool KeyspaceMsg::BuildFrom(KeyspaceOp* op)
{
	bool ret;
	
	if (op->type == KeyspaceOp::GET)
		Init(KEYSPACE_GET);
	else if (op->type == KeyspaceOp::SET)
		Init(KEYSPACE_SET);
	else if (op->type == KeyspaceOp::TEST_AND_SET)
		Init(KEYSPACE_TESTANDSET);
	else if (op->type == KeyspaceOp::ADD)
		Init(KEYSPACE_ADD);
	else if (op->type == KeyspaceOp::DELETE)
		Init(KEYSPACE_DELETE);
	else
		ASSERT_FAIL();
	
	ret = true;
	ret &= key.Set(op->key);
	if (op->type == KeyspaceOp::SET || op->type == KeyspaceOp::TEST_AND_SET)
		ret &= value.Set(op->value);
	if (op->type == KeyspaceOp::TEST_AND_SET)
		ret &= test.Set(op->test);
	if (op->type == KeyspaceOp::ADD)
		num = op->num;
	return ret;
}
