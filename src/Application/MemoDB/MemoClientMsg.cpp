#include "MemoClientMsg.h"
#include "Common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void MemoClientMsg::GetRequest(ByteString key_)
{
	type = GET_REQUEST;
	key = key_;
}

void MemoClientMsg::GetResponse(char exec_, char response_, ByteString value_)
{
	type = GET_RESPONSE;
	exec = exec_;
	response = response_;
	value = value_;
}

void MemoClientMsg::GetResponse(char exec_, char response_)
{
	type = GET_RESPONSE;
	exec = exec_;
	response = response_;
}

void MemoClientMsg::SetRequest(ByteString key_, ByteString value_)
{
	type = SET_REQUEST;
	key = key_;
	value = value_;
}

void MemoClientMsg::SetResponse(char exec_, char response_)
{
	type = SET_RESPONSE;
	exec = exec_;
	response = response_;
}

void MemoClientMsg::TestAndSetRequest(ByteString key_, ByteString test_, ByteString value_)
{
	type = TESTANDSET_REQUEST;
	key = key_;
	test = test_;
	value = value_;
}

void MemoClientMsg::TestAndSetResponse(char exec_, char response_)
{
	type = TESTANDSET_RESPONSE;
	exec = exec_;
	response = response_;
}

void MemoClientMsg::Error()
{
	type = ERROR;
}

int MemoClientMsg::Read(ByteString data, MemoClientMsg* msg)
{
	char			protocol, type;
	int				nread;
	char*			pos;
	ByteString		key, value, test;
	
	
	/*	format for memo messages, numbers are textual:
		get request:		<type>:<key-length>:<key>
		get response:		<type>:<exec>:<response>:value-length>:<value>
		set request:		<type>:<key-length>:<key>:<value-length>:value>
		set response		<type>:<exec>:<reponse>
		testandset request:	<type>:<key-length>:<key>:<test-length>:<test-value>:<value-length>:<value>
		testandset response:<type>:<exec>:<response>
	*/
	
#define CheckOverflow()		if ((pos - data.buffer) >= data.length) return 0;
#define ReadNumber(num)		(num) = strntol(pos, data.length - (pos - data.buffer), &nread); \
							if (nread < 1) return false; pos += nread;
#define ReadChar(c)			(c) = *pos; pos++;
#define ReadSeparator()		if (*pos != ':') return 0; pos++;
#define ValidateLength()	if (pos - data.buffer > data.length) return 0;

	pos = data.buffer;
	CheckOverflow();
	ReadChar(protocol); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadChar(type); CheckOverflow();
	
	if (protocol != PROTOCOL_MEMODB)
		return 0;
	
	if (type != GET_REQUEST	&&
		type != GET_RESPONSE &&
		type != SET_REQUEST &&
		type != SET_RESPONSE &&
		type != TESTANDSET_REQUEST &&
		type != TESTANDSET_RESPONSE)
			return 0;

	ReadSeparator(); CheckOverflow();
	
	if (type == GET_REQUEST)
	{
		ReadNumber(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		
		key.buffer = pos;
		key.size = data.size - (pos - data.buffer);
		pos += key.length;
		
		ValidateLength();
		
		msg->GetRequest(key);
		return pos - data.buffer;
	}
	else if (type == GET_RESPONSE)
	{
		// not required on server side
		return 0;
	}
	else if (type == SET_REQUEST)
	{
		ReadNumber(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		
		key.buffer = pos;
		key.size = data.size - (pos - data.buffer);
		pos += key.length;
		
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(value.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		
		value.buffer = pos;
		value.size = data.size - (pos - data.buffer);
		pos += value.length;
		
		ValidateLength();
		
		msg->SetRequest(key, value);
		return pos - data.buffer;
	}
	else if (type == SET_RESPONSE)
	{
		// not required on server side
		return 0;
	}
	else if (type == TESTANDSET_REQUEST)
	{
		ReadNumber(key.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		
		key.buffer = pos;
		key.size = data.size - (pos - data.buffer);
		pos += key.length;
		
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(test.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		
		test.buffer = pos;
		test.size = data.size - (pos - data.buffer);
		pos += test.length;
		
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(value.length); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		
		value.buffer = pos;
		value.size = data.size - (pos - data.buffer);
		pos += value.length;

		ValidateLength();
		
		msg->TestAndSetRequest(key, test, value);
		return pos - data.buffer;
	}
	else if (type == TESTANDSET_RESPONSE)
	{
		// not required on server side
		return 0;
	}
	
	return 0;
}

bool MemoClientMsg::Write(MemoClientMsg* msg, ByteString& data)
{
#define WRITE_VALUE()	if (required + msg->value.length > data.size) return -1; \
						memcpy(data.buffer + required, msg->value.buffer, msg->value.length); \
						required += msg->value.length;

	int required;
	
	if (msg->type == GET_REQUEST)
	{
		// not required on server side
		return -1;
	}
	else if (msg->type == GET_RESPONSE)
	{
		// get response:	<type>:<exec>:<response>:value-length>:<value>
		
		if (msg->exec == EXEC_OK)
		{
			if (msg->response == RESPONSE_OK)
			{
				required  = snprintf(data.buffer, data.size, "%c:%c:%c:%c:%d:",
					PROTOCOL_MEMODB, msg->type, msg->exec, msg->response, msg->value.length);
				WRITE_VALUE();
			}
			else
				required = snprintf(data.buffer, data.size, "%c:%c:%c:%c",
					PROTOCOL_MEMODB, msg->type, msg->exec, msg->response);
		}
		else
			required = snprintf(data.buffer, data.size, "%c:%c", msg->type, msg->exec);
	}
	else if (msg->type == SET_REQUEST)
	{
		// not required on server side
		return -1;
	}
	else if (msg->type == SET_RESPONSE)
	{
		// set response		<type>:<exec>:<reponse>
		
		if (msg->exec == EXEC_OK)
			required = snprintf(data.buffer, data.size, "%c:%c:%c:%c",
				PROTOCOL_MEMODB, msg->type, msg->exec, msg->response);
		else
			required = snprintf(data.buffer, data.size, "%c:%c:%c",
				PROTOCOL_MEMODB, msg->type, msg->exec);
	}
	else if (msg->type == TESTANDSET_REQUEST)
	{
		// not required on server side
		return -1;
	}
	else if (msg->type == TESTANDSET_RESPONSE)
	{
		// testandset response:		<type>:<exec>:<response>
		
		if (msg->exec == EXEC_OK)
			required = snprintf(data.buffer, data.size, "%c:%c:%c:%c",
				PROTOCOL_MEMODB, msg->type, msg->exec, msg->response);
		else
			required = snprintf(data.buffer, data.size, "%c:%c:%c",
				PROTOCOL_MEMODB, msg->type, msg->exec);
	}
	else if (msg->type == ERROR)
	{
		required = snprintf(data.buffer, data.size, "%c:%c", PROTOCOL_MEMODB, msg->type);
	}
	else
		return -1;

	required++; // for tailing '\n'
	
	if (required > data.size)
		return false;

	data.buffer[required - 1] = '\n';
		
	data.length = required;
	return true;
}
