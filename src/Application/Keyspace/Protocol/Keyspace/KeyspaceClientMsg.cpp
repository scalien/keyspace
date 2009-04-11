#include "KeyspaceClientMsg.h"
#include <stdio.h>
#include <inttypes.h>

void KeyspaceClientMsg::Init(char type_)
{
	type = type_;
}

bool KeyspaceClientMsg::Get(ByteString key_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_GET);	
	
	key.Reallocate(key_.length);
	key.Set(key_);
	return true;
}

bool KeyspaceClientMsg::DirtyGet(ByteString key_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_DIRTYGET);	
	
	key.Reallocate(key_.length);
	key.Set(key_);
	return true;
}

bool KeyspaceClientMsg::List(ByteString prefix_, uint64_t count_)
{
	if (prefix_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_LIST);
	
	key.Reallocate(prefix_.length);
	key.Set(prefix_);
	count = count_;
	return true;
}

bool KeyspaceClientMsg::DirtyList(ByteString prefix_, uint64_t count_)
{
	if (prefix_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_DIRTYLIST);
	
	key.Reallocate(prefix_.length);
	key.Set(prefix_);
	count = count_;
	return true;
}

bool KeyspaceClientMsg::Set(ByteString key_, ByteString value_)
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
	
bool KeyspaceClientMsg::TestAndSet(ByteString key_, ByteString test_, ByteString value_)
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

bool KeyspaceClientMsg::Add(ByteString key_, int64_t num_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_ADD);
	
	key.Reallocate(key_.length);
	key.Set(key_);
	num = num_;
	return true;
}

bool KeyspaceClientMsg::Delete(ByteString key_)
{
	if (key_.length > KEYSPACE_KEY_SIZE)
		return false;

	Init(KEYSPACECLIENT_DELETE);
	
	key.Reallocate(key_.length);
	key.Set(key_);
	return true;
}

bool KeyspaceClientMsg::Submit()
{
	Init(KEYSPACECLIENT_SUBMIT);
	return true;
}
	
bool KeyspaceClientMsg::Read(ByteString data)
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
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		key.buffer = pos;
		pos += key.length;
		CheckOverflow();
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

bool KeyspaceClientMsg::Write(ByteString& data)
{
	int required;
	
	if (type == KEYSPACECLIENT_GET || type == KEYSPACECLIENT_DIRTYGET)
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
	else if (type == KEYSPACECLIENT_SUBMIT)
			required = snprintf(data.buffer, data.size, "*");
	else
		return false;
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;

}
