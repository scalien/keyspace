#include "CatchupMsg.h"

void CatchupMsg::Init(char type_)
{
	type = type_;
}

void CatchupMsg::KeyValue(ByteString& key_, ByteString& value_)
{
	Init(CATCHUP_KEY_VALUE);
	key.Set(key_);
	value.Set(value_);
}

void CatchupMsg::Commit(uint64_t paxosID_)
{
	Init(CATCHUP_COMMIT);
	paxosID = paxosID_;
	
}

bool CatchupMsg::Read(const ByteString& data)
{
	int read;
	
	if (data.length < 1)
		return false;
	
	switch (data.buffer[0])
	{
		case CATCHUP_KEY_VALUE:
			read = snreadf(data.buffer, data.length, "%c:%M:%M",
						   &type, &key, &value);
			break;
		case CATCHUP_COMMIT:
			read = snreadf(data.buffer, data.length, "%c:%U",
						   &type, &paxosID);
			break;
		default:
			return false;
	}
	
	return (read == (signed)data.length ? true : false);
}
	
bool CatchupMsg::Write(ByteString& data)
{
	switch (type)
	{
		case CATCHUP_KEY_VALUE:
			return data.Writef("%c:%M:%M",
							   type, &key, &value);
			break;
		case CATCHUP_COMMIT:
			return data.Writef("%c:%U",
							   type, paxosID);
			break;
		default:
			return false;
	}
}
