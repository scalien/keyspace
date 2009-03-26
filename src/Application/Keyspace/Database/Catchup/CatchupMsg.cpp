#include "CatchupMsg.h"
#include <stdio.h>

void CatchupMsg::Init(char type_)
{
	type = type_;
}

void CatchupMsg::Commit()
{
	Init(CATCHUP_COMMIT);
}

void CatchupMsg::Rollback()
{
	Init(CATCHUP_ROLLBACK);
}

void CatchupMsg::KeyValue(ByteString& key_, ByteString& value_)
{
	Init(KEY_VALUE);
	key.Set(key_);
	value.Set(value_);
}

void CatchupMsg::DBCommand(ulong64 paxosID_, ByteString& dbCommand_)
{
	Init(DB_COMMAND);
	paxosID = paxosID_;
	dbCommand.Set(dbCommand_);
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
#define ReadSeparator()		if (*pos != '#') return false; pos++;
#define ValidateLength()	if ((pos - data.buffer) != data.length) return false;

	pos = data.buffer;
	CheckOverflow();
	ReadChar(type);
	
	if (type == CATCHUP_COMMIT || CATCHUP_ROLLBACK)
	{
		ValidateLength();
		Init(type);
		return true;
	}
	else if (type == KEY_VALUE)
	{
		CheckOverflow();
		ReadNumber(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		
		key.buffer = pos;
		key.size = key.length;
		
		pos += key.length;
		ReadNumber(value.length); CheckOverflow();
		ReadSeparator();

		value.buffer = pos;
		value.size = value.length;

		pos += value.length;
		ValidateLength();
		KeyValue(key, value);
		return true;
	}
	else if (type == DB_COMMAND)
	{
		CheckOverflow();
		ReadNumber(paxosID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(dbCommand.length); CheckOverflow();
		ReadSeparator();
		
		dbCommand.buffer = pos;
		dbCommand.size = dbCommand.length;
		
		pos += dbCommand.length;
		ValidateLength();
		DBCommand(paxosID, dbCommand);
		return true;
	}
	else
		ASSERT_FAIL();
}
	
bool CatchupMsg::Write(ByteString& data)
{
	int		required, size;
	char*	p;
	
	p = data.buffer + data.length;
	size = data.size - data.length;
	
	if (type == CATCHUP_COMMIT || CATCHUP_ROLLBACK)
		required = snprintf(p, size, "%c", type);
	else if (type == KEY_VALUE)
		required = snprintf(p, size, "%c:%d:%.*s:%d:%.*s", type,
			key.length, key.length, key.buffer, value.length, value.length, value.buffer);
	else if (type == DB_COMMAND)
		required = snprintf(p, size, "%c:%llu:%d:%.*s", type,
			paxosID, dbCommand.length, dbCommand.length, dbCommand.buffer);
	else
		return false;
	
	if (required > size)
		return false;
		
	data.length += required;
	return true;
}
