#include "KeyspaceResponse.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientResp.h"

using namespace Keyspace;

bool Response::Read(const ByteString& data_)
{
	bool		ret;
	char		cmd;
	uint64_t	cmdID;
	ByteString	tmp;

	data = data_;
	pos = data.buffer;
	separator = ':';

	msg.Clear();
	key.Clear();
	value.Clear();
	
	ret = ReadChar(cmd);
	ret = ret && ReadSeparator();
	ret = ret && ReadUint64(cmdID);
	if (!ret)
		return false;
	
	id = cmdID;
	type = cmd;
	
	if (cmd == KEYSPACECLIENT_NOT_MASTER)
		return ValidateLength();
	
	if (cmd == KEYSPACECLIENT_FAILED)
		return ValidateLength();
	
	if (cmd == KEYSPACECLIENT_LIST_END)
		return ValidateLength();
	
	if (cmd == KEYSPACECLIENT_OK)
	{
		ret = ReadMessage(tmp);
		if (ret)
			value.Append(tmp.buffer, tmp.length);

		return ValidateLength();
	}
	
	if (cmd == KEYSPACECLIENT_LIST_ITEM)
	{
		ret = ReadMessage(tmp);
		if (ret)
			key.Append(tmp.buffer, tmp.length);
		ret = ret && ValidateLength();
		return ret;
	}
	
	if (cmd == KEYSPACECLIENT_LISTP_ITEM)
	{
		ret = ReadMessage(tmp);
		if (ret)
			key.Append(tmp.buffer, tmp.length);
		ret = ret && ReadMessage(tmp);
		if (ret)
			value.Append(tmp.buffer, tmp.length);
		
		ret = ret && ValidateLength();
		return ret;
	}
	
	return false;
}

bool Response::CheckOverflow()
{
	if ((pos - data.buffer) >= (int) data.length || pos < data.buffer)
		return false;
	return true;
}

bool Response::ReadUint64(uint64_t& num)
{
	unsigned	nread;
	
	if (!CheckOverflow())
		return false;

	num = strntouint64(pos, data.length - (pos - data.buffer), &nread);
	if (nread < 1)
		return false;
	
	pos += nread;
	return true;
}

bool Response::ReadChar(char& c)
{
	if (!CheckOverflow())
		return false;

	c = *pos; 
	pos++;
	
	return true;
}

bool Response::ReadSeparator()
{
	if (!CheckOverflow())
		return false;
	
	if (*pos != separator) 
		return false; 
	
	pos++;
	return true;
}

bool Response::ReadMessage(ByteString& bs)
{
	bool		ret;
	uint64_t	num;
	
	ret = true;
	ret = ret && ReadSeparator();
	ret = ret && ReadUint64(num);
	ret = ret && ReadSeparator();
	ret = ret && ReadData(bs, num);
	
	return ret;
}

bool Response::ReadData(ByteString& bs, uint64_t length)
{
	if (pos - length > data.buffer + data.length)
		return false;
	
	bs.buffer = pos;
	bs.length = length;

	pos += length;
	
	return true;
}

bool Response::ValidateLength()
{
	if ((pos - data.buffer) != (int) data.length) 
		return false;
	
	return true;
}

