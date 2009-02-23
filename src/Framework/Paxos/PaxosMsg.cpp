#include "PaxosMsg.h"
#include "System/Common.h"
#include <stdlib.h>
#include <stdio.h>

void PaxosMsg::Init(ulong64 paxosID_, char type_)
{
	paxosID = paxosID_;
	type = type_;
}

bool PaxosMsg::PrepareRequest(ulong64 paxosID_, ulong64 n_)
{
	Init(paxosID_, PREPARE_REQUEST);
	n = n_;

	return true;
}

bool PaxosMsg::PrepareResponse(ulong64 paxosID_, ulong64 n_, char response_)
{
	Init(paxosID_, PREPARE_RESPONSE);
	n = n_;
	response = response_;

	return true;
}

bool PaxosMsg::PrepareResponse(ulong64 paxosID_, ulong64 n_, char response_,
	ulong64 n_accepted_, ByteString value_)
{
	Init(paxosID_, PREPARE_RESPONSE);
	n = n_;
	response = response_;
	n_accepted = n_accepted_;
	if (!value.Set(value_))
		return false;
	
	return true;
}

bool PaxosMsg::ProposeRequest(ulong64 paxosID_, ulong64 n_, ByteString value_)
{
	Init(paxosID_, PROPOSE_REQUEST);
	n = n_;
	if (!value.Set(value_))
		return false;
	
	return true;
}

bool PaxosMsg::ProposeResponse(ulong64 paxosID_, ulong64 n_, char response_)
{
	Init(paxosID_, PROPOSE_RESPONSE);
	n = n_;
	response = response_;

	return true;
}

bool PaxosMsg::LearnChosen(ulong64 paxosID_, ByteString value_)
{
	Init(paxosID_, LEARN_CHOSEN);
	if (!value.Set(value_))
		return false;
	
	return true;
}

bool PaxosMsg::Read(ByteString& data)
{
	ulong64		paxosID;
	char		type;
	
	int			nread;
	char		*pos;
	
	/*	format for paxos messages, numbers are textual:
		<paxosID>#<type>#<<<type specific>>>
	*/
	
#define CheckOverflow()		if ((pos - data.buffer) >= data.length) return false;
#define ReadNumber(num)		(num) = strntoulong64(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadChar(c)			(c) = *pos; pos++;
#define ReadSeparator()		if (*pos != '#') return false; pos++;
#define ValidateLength()	if ((pos - data.buffer) != data.length) return false;

	pos = data.buffer;
	CheckOverflow();
	ReadNumber(paxosID); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadChar(type);	CheckOverflow();
	
	if (type != PREPARE_REQUEST	&&
		type != PREPARE_RESPONSE &&
		type != PROPOSE_REQUEST &&
		type != PROPOSE_RESPONSE &&
		type != LEARN_CHOSEN)
			return false;
	
	ReadSeparator(); CheckOverflow();
	
	if (type == PREPARE_REQUEST)
	{
		// <<<type specific>>> := <n>
		ulong64 n;
		
		ReadNumber(n);
		
		ValidateLength();
		PrepareRequest(paxosID, n);
		return true;
	}
	else if (type == PREPARE_RESPONSE)
	{
		// <<<type specific>>> := <n>#<response>#<n_accepted>#<length>#<value>
		// the <n_accepted>#<length>#<value> is only present if
		// response == PREPARE_PREVIOUSLY_ACCEPTED
		ulong64	n, n_accepted;
		char	response;
		int		length;
		
		ReadNumber(n); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadChar(response);
	
		if (response != PREPARE_REJECTED &&
			response != PREPARE_PREVIOUSLY_ACCEPTED &&
			response != PREPARE_CURRENTLY_OPEN)
				return false;
		
		if (response == PREPARE_REJECTED || response == PREPARE_CURRENTLY_OPEN)
		{
			ValidateLength();
			PrepareResponse(paxosID, n, response);
			return true;
		}

		CheckOverflow();		
		ReadSeparator(); CheckOverflow();
		ReadNumber(n_accepted); CheckOverflow();
		ReadSeparator(); CheckOverflow();		
		ReadNumber(length); CheckOverflow();
		ReadSeparator();
		
		if (pos - data.buffer != data.length - length)
			return false;

		PrepareResponse(paxosID, n, response, n_accepted,
			ByteString(data.size - (pos - data.buffer), length, pos));
		return true;
	}
	else if (type == PROPOSE_REQUEST)
	{
		// <<<type specific>>> := <n>#<length>#<value>
		ulong64	n;
		int		length;
		
		ReadNumber(n); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(length); CheckOverflow();
		ReadSeparator();
		
		if (pos - data.buffer != data.length - length)
			return false;
		
		ProposeRequest(paxosID, n, ByteString(data.size - (pos - data.buffer), length, pos));
		return true;
	}
	else if (type == PROPOSE_RESPONSE)
	{
		// <<<type specific>>> := <n>#<response>
		ulong64	n;
		char	response;
		
		ReadNumber(n); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadChar(response);
		
		if (response != PROPOSE_REJECTED &&
			response != PROPOSE_ACCEPTED)
				return false;
		
		ValidateLength();
		ProposeResponse(paxosID, n, response);
		return true;
	}
	else if (type == LEARN_CHOSEN)
	{
		// <<<type specific>>> := <length>#<value>
		int		length;
		
		ReadNumber(length); CheckOverflow();
		ReadSeparator();
		
		if (pos - data.buffer != data.length - length)
			return false;
		
		LearnChosen(paxosID, ByteString(data.size - (pos - data.buffer), length, pos));
		return true;
	}
	
	return false;
}

bool PaxosMsg::Write(ByteString& data)
{
#define WRITE_VALUE()	if (required + value.length > data.size) return false; \
						memcpy(data.buffer + required, value.buffer, value.length); \
						required += value.length;
	
	int required;
	
	if (type == PREPARE_REQUEST)
	{
		required = snprintf(data.buffer, data.size, "%llu#%c#%llu", paxosID, type, n);
	}
	else if (type == PREPARE_RESPONSE)
	{
		if (response == PREPARE_REJECTED || response == PREPARE_CURRENTLY_OPEN)
		{
			required = snprintf(data.buffer, data.size, "%llu#%c#%llu#%c", paxosID, type,
				n, response);
		}
		else
		{
			required = snprintf(data.buffer, data.size, "%llu#%c#%llu#%c#%llu#%d#", paxosID,
				type, n, response, n_accepted, value.length);
			WRITE_VALUE();
		}
	}
	else if (type == PROPOSE_REQUEST)
	{
		required = snprintf(data.buffer, data.size, "%llu#%c#%llu#%d#", paxosID, type, n,
			value.length);
		WRITE_VALUE();
	}
	else if (type == PROPOSE_RESPONSE)
	{
		required = snprintf(data.buffer, data.size, "%llu#%c#%llu#%c", paxosID, type, n, response);
	}
	else if (type == LEARN_CHOSEN)
	{
		required = snprintf(data.buffer, data.size, "%llu#%c#%d#", paxosID, type, value.length);
		WRITE_VALUE();
	}
	else
		return false;
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;
}
