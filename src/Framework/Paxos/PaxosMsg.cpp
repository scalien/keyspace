#include "PaxosMsg.h"
#include "System/Common.h"
#include <stdlib.h>
#include <stdio.h>

void PaxosMsg::Init(long paxosID_, char type_)
{
	paxosID = paxosID_;
	type = type_;
}

void PaxosMsg::PrepareRequest(long paxosID_, long n_)
{
	Init(paxosID_, PREPARE_REQUEST);
	n = n_;
}

void PaxosMsg::PrepareResponse(long paxosID_, long n_, char response_)
{
	Init(paxosID_, PREPARE_RESPONSE);
	n = n_;
	response = response_;
}

bool PaxosMsg::PrepareResponse(long paxosID_, long n_, char response_,
	long n_accepted_, ByteString value_)
{
	Init(paxosID_, PREPARE_RESPONSE);
	n = n_;
	response = response_;
	n_accepted = n_accepted_;
	if (!value.Set(value_))
		return false;
	
	return true;
}

bool PaxosMsg::ProposeRequest(long paxosID_, long n_, ByteString value_)
{
	Init(paxosID_, PROPOSE_REQUEST);
	n = n_;
	if (!value.Set(value_))
		return false;
	
	return true;
}

void PaxosMsg::ProposeResponse(long paxosID_, long n_, char response_)
{
	Init(paxosID_, PROPOSE_RESPONSE);
	n = n_;
	response = response_;
}

bool PaxosMsg::LearnChosen(long paxosID_, ByteString value_)
{
	Init(paxosID_, LEARN_CHOSEN);
	if (!value.Set(value_))
		return false;
	
	return true;
}

bool PaxosMsg::Read(ByteString data, PaxosMsg* msg)
{
	long		paxosID;
	char		type;
	
	int			nread;
	char		*pos;
	
	/*	format for paxos messages, numbers are textual:
		<paxosID>#<type>#<<<type specific>>>
	*/
	
#define CheckOverflow()		if ((pos - data.buffer) >= data.length) return false;
#define ReadNumber(num)		(num) = strntol(pos, data.length - (pos - data.buffer), &nread); \
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
		long n;
		
		ReadNumber(n);
		
		ValidateLength();
		msg->PrepareRequest(paxosID, n);
		return true;
	}
	else if (type == PREPARE_RESPONSE)
	{
		// <<<type specific>>> := <n>#<response>#<n_accepted>#<length>#<value>
		// the <n_accepted>#<length>#<value> is only present if
		// response == PREPARE_PREVIOUSLY_ACCEPTED
		long	n, n_accepted;
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
			msg->PrepareResponse(paxosID, n, response);
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

		msg->PrepareResponse(paxosID, n, response, n_accepted,
			ByteString(data.size - (pos - data.buffer), length, pos));
		return true;
	}
	else if (type == PROPOSE_REQUEST)
	{
		// <<<type specific>>> := <n>#<length>#<value>
		long	n;
		int		length;
		
		ReadNumber(n); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(length); CheckOverflow();
		ReadSeparator();
		
		if (pos - data.buffer != data.length - length)
			return false;
		
		msg->ProposeRequest(paxosID, n, ByteString(data.size - (pos - data.buffer), length, pos));
		return true;
	}
	else if (type == PROPOSE_RESPONSE)
	{
		// <<<type specific>>> := <n>#<response>
		long	n;
		char	response;
		
		ReadNumber(n); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadChar(response);
		
		if (response != PROPOSE_REJECTED &&
			response != PROPOSE_ACCEPTED)
				return false;
		
		ValidateLength();
		msg->ProposeResponse(paxosID, n, response);
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
		
		msg->LearnChosen(paxosID, ByteString(data.size - (pos - data.buffer), length, pos));
		return true;
	}
	
	return false;
}

bool PaxosMsg::Write(PaxosMsg* msg, ByteString& data)
{
#define WRITE_VALUE()	if (required + msg->value.length > data.size) return false; \
						memcpy(data.buffer + required, msg->value.buffer, msg->value.length); \
						required += msg->value.length;
	
	int required;
	
	if (msg->type == PREPARE_REQUEST)
	{
		required = snprintf(data.buffer, data.size, "%ld#%c#%ld", msg->paxosID, msg->type, msg->n);
	}
	else if (msg->type == PREPARE_RESPONSE)
	{
		if (msg->response == PREPARE_REJECTED || msg->response == PREPARE_CURRENTLY_OPEN)
		{
			required = snprintf(data.buffer, data.size, "%ld#%c#%ld#%c", msg->paxosID, msg->type,
				msg->n, msg->response);
		}
		else
		{
			required = snprintf(data.buffer, data.size, "%ld#%c#%ld#%c#%ld#%d#", msg->paxosID,
				msg->type, msg->n, msg->response, msg->n_accepted, msg->value.length);
			WRITE_VALUE();
		}
	}
	else if (msg->type == PROPOSE_REQUEST)
	{
		required = snprintf(data.buffer, data.size, "%ld#%c#%ld#%d#", msg->paxosID, msg->type,
			msg->n, msg->value.length);
		WRITE_VALUE();
	}
	else if (msg->type == PROPOSE_RESPONSE)
	{
		required = snprintf(data.buffer, data.size, "%ld#%c#%ld#%c", msg->paxosID, msg->type,
			msg->n, msg->response);
	}
	else if (msg->type == LEARN_CHOSEN)
	{
		required = snprintf(data.buffer, data.size, "%ld#%c#%d#", msg->paxosID, msg->type,
			msg->value.length);
		WRITE_VALUE();
	}
	else
		return false;
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;
}
