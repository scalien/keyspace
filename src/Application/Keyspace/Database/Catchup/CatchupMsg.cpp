#include "CatchupMsg.h"
#include <stdio.h>

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

void CatchupMsg::Commit(ulong64 paxosID_)
{
	Init(CATCHUP_COMMIT);
	paxosID = paxosID_;
	
}

bool CatchupMsg::Read(ByteString data)
{
	char		type;
	int			nread;
	char		*pos;
	ulong64		paxosID;
	ByteString	key, value, dbCommand;
	
#define CheckOverflow()		if ((pos - data.buffer) >= data.length) return false;
#define ReadNumber(num)		(num) = strntoulong64(pos, data.length - (pos - data.buffer), &nread); \
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
		ReadNumber(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		
		key.buffer = pos;
		key.size = key.length;
		
		pos += key.length;
		ReadSeparator(); CheckOverflow();
		ReadNumber(value.length); CheckOverflow();
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
		ReadNumber(paxosID);
		
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
		required = snprintf(data.buffer, data.size, "%c:%llu", type, paxosID);
	else
		return false;
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;
}
