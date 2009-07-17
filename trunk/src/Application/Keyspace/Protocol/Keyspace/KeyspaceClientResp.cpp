#include "KeyspaceClientResp.h"

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
	type = KEYSPACECLIENT_NOT_MASTER;
	cmdID = cmdID_;
	key.length = 0;
	value.length = 0;
}

void KeyspaceClientResp::ListItem(uint64_t cmdID_, ByteString key_)
{
	type = KEYSPACECLIENT_LIST_ITEM;
	cmdID = cmdID_;
	key = key_;
	value.length = 0;
}

void KeyspaceClientResp::ListPItem(uint64_t cmdID_, ByteString key_, ByteString value_)
{
	type = KEYSPACECLIENT_LISTP_ITEM;
	cmdID = cmdID_;
	key = key_;
	value = value_;
}

void KeyspaceClientResp::ListEnd(uint64_t cmdID_)
{
	type = KEYSPACECLIENT_LIST_END;
	cmdID = cmdID_;
	key.length = 0;
	value.length = 0;
}

bool KeyspaceClientResp::Write(ByteString& data)
{
	if (key.length > 0 && value.length > 0)
		return data.Writef("%c:%U:%M:%M",
					       type, cmdID, &key, &value);
	else if (key.length > 0)
		return data.Writef("%c:%U:%M",
					       type, cmdID, &key);
	else if (value.length > 0)
		return data.Writef("%c:%U:%M",
					       type, cmdID, &value);
	else
		return data.Writef("%c:%U",
					       type, cmdID);
}
