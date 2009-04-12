#include "CatchupMsg.h"
#include <stdio.h>
#include <inttypes.h>

void CatchupMsg::Init(char type_)
{
	type = type_;
}

void CatchupMsg::KeyValue(ByteString& key_, ByteString& value_)
{
	Init(KEY_VALUE);
	key.Set(key_);
	value.Set(value_);
}

void CatchupMsg::Commit(uint64_t paxosID_)
{
	Init(CATCHUP_COMMIT);
	paxosID = paxosID_;
	
}

bool CatchupMsg::Read(ByteString data)
{
	char		type;
	unsigned	nread;
	char		*pos;
	uint64_t	paxosID;
	ByteString	key, value, dbCommand;
	
#define CheckOverflow()		if ((pos - data.buffer) >= data.length || pos < data.buffer) return false;
#define ReadUint64_t(num)		(num) = strntouint64_t(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadChar(c)			(c) = *pos; pos++;
#define ReadSeparator()		if (*pos != ':') return false; pos++;
#define ValidateLength()	if ((pos - data.buffer) != data.length) return false;

	pos = data.buffer;
	CheckOverflow();
	ReadChar(type);
	
	if (type == KEY_VALUE)
	{
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64_t(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		
		key.buffer = pos;
		key.size = key.length;
		
		pos += key.length;
		ReadSeparator(); CheckOverflow();
		ReadUint64_t(value.length); CheckOverflow();
		ReadSeparator();

		value.buffer = pos;
		value.size = value.length;

		pos += value.length;
		ValidateLength();
		KeyValue(key, value);
		return true;
	}
	else if (type == CATCHUP_COMMIT)
	{
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64_t(paxosID);
		
		ValidateLength();
		Commit(paxosID);
		return true;
	}
	else
		ASSERT_FAIL();
}
	
bool CatchupMsg::Write(ByteString& data)
{
	int		required;
	
	if (type == KEY_VALUE)
		required = snprintf(data.buffer, data.size, "%c:%d:%.*s:%d:%.*s", type,
			key.length, key.length, key.buffer, value.length, value.length, value.buffer);
	else if (type == CATCHUP_COMMIT)
		required = snprintf(data.buffer, data.size, "%c:%" PRIu64 "", type, paxosID);
	else
		return false;
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;
}
