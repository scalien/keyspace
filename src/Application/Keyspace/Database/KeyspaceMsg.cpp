#include "KeyspaceMsg.h"
#include "KeyspaceService.h"
#include <stdio.h>
#include <inttypes.h>

void KeyspaceMsg::Init(char type_)
{
	type = type_;
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

void KeyspaceMsg::Prune(ByteString prefix_)
{
	Init(KEYSPACE_PRUNE);
	
	prefix.Set(prefix_);
}
	
bool KeyspaceMsg::Read(ByteString& data, unsigned &n)
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
#define ValidateLength()	if ((pos - data.buffer) != data.length) return false;

	pos = data.buffer;
	CheckOverflow();
	ReadChar(type); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	
	if (type == KEYSPACE_SET)
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
	else if (type == KEYSPACE_PRUNE)
	{
		ReadUint64_t(prefix.length); CheckOverflow();
		ReadSeparator(); if (prefix.length > 0) CheckOverflow();
		prefix.buffer = pos;
		pos += prefix.length;
		
		if (pos > data.buffer + data.length)
			return false;
		
		Prune(ByteString(prefix.length, prefix.length, prefix.buffer));
		n = pos - data.buffer;
		return true;
	}
	
	return false;
}

bool KeyspaceMsg::Write(ByteString& data)
{
	int required;
	
	if (type == KEYSPACE_SET)
		required = snwritef(data.buffer, data.size, "%c:%d:%B:%d:%B", type,
			key.length, key.length, key.buffer,
			value.length, value.length, value.buffer);
	else if (type == KEYSPACE_TESTANDSET)
		required = snwritef(data.buffer, data.size, "%c:%d:%B:%d:%B:%d:%B", type,
			key.length, key.length, key.buffer,
			test.length, test.length, test.buffer,
			value.length, value.length, value.buffer);
	else if (type == KEYSPACE_ADD)
		required = snwritef(data.buffer, data.size, "%c:%d:%B:%I", type,
			key.length, key.length, key.buffer, num);
	else if (type == KEYSPACE_DELETE)
		required = snwritef(data.buffer, data.size, "%c:%d:%B", type,
			key.length, key.length, key.buffer);
	else if (type == KEYSPACE_PRUNE)
		required = snwritef(data.buffer, data.size, "%c:%d:%B", type,
			prefix.length, prefix.length, prefix.buffer);
	else
		return false;
	
	if (required < 0 || (unsigned)required > data.size)
		return false;
		
	data.length = required;
	return true;
}

bool KeyspaceMsg::FromKeyspaceOp(KeyspaceOp* op)
{
	bool ret;
	
	if (op->type == KeyspaceOp::SET)
		Init(KEYSPACE_SET);
	else if (op->type == KeyspaceOp::TEST_AND_SET)
		Init(KEYSPACE_TESTANDSET);
	else if (op->type == KeyspaceOp::ADD)
		Init(KEYSPACE_ADD);
	else if (op->type == KeyspaceOp::DELETE)
		Init(KEYSPACE_DELETE);
	else if (op->type == KeyspaceOp::PRUNE)
		Init(KEYSPACE_PRUNE);
	else
		ASSERT_FAIL();
	
	ret = true;
	
	if (op->type == KeyspaceOp::PRUNE)
		ret &= prefix.Set(op->prefix);
	else
		ret &= key.Set(op->key);
	
	if (op->type == KeyspaceOp::SET || op->type == KeyspaceOp::TEST_AND_SET)
		ret &= value.Set(op->value);
	if (op->type == KeyspaceOp::TEST_AND_SET)
		ret &= test.Set(op->test);
	if (op->type == KeyspaceOp::ADD)
		num = op->num;
	return ret;
}
