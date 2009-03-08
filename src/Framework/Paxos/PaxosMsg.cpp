#include "PaxosMsg.h"
#include "System/Common.h"
#include <stdlib.h>
#include <stdio.h>

void PaxosMsg::Init(ulong64 paxosID_, char type_)
{
	paxosID = paxosID_;
	type = type_;
}

bool PaxosMsg::PrepareRequest(ulong64 paxosID_, ulong64 proposalID_)
{
	Init(paxosID_, PREPARE_REQUEST);
	proposalID = proposalID_;

	return true;
}

bool PaxosMsg::PrepareResponse(ulong64 paxosID_, ulong64 proposalID_, char response_)
{
	Init(paxosID_, PREPARE_RESPONSE);
	proposalID = proposalID_;
	response = response_;

	return true;
}

bool PaxosMsg::PrepareResponse(ulong64 paxosID_, ulong64 proposalID_, char response_,
	ulong64 acceptedProposalID_, ByteString value_)
{
	Init(paxosID_, PREPARE_RESPONSE);
	proposalID = proposalID_;
	response = response_;
	acceptedProposalID = acceptedProposalID_;
	if (!value.Set(value_))
		return false;
	
	return true;
}

bool PaxosMsg::ProposeRequest(ulong64 paxosID_, ulong64 proposalID_, ByteString value_)
{
	Init(paxosID_, PROPOSE_REQUEST);
	proposalID = proposalID_;
	if (!value.Set(value_))
		return false;
	
	return true;
}

bool PaxosMsg::ProposeResponse(ulong64 paxosID_, ulong64 proposalID_, char response_)
{
	Init(paxosID_, PROPOSE_RESPONSE);
	proposalID = proposalID_;
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

bool PaxosMsg::RequestChosen(ulong64 paxosID_)
{
	Init(paxosID_, REQUEST_CHOSEN);
	
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
	ReadChar(type);
		
	if (type == REQUEST_CHOSEN)
	{
		ValidateLength();
		RequestChosen(paxosID);
		return true;
	}
	
	CheckOverflow();
	ReadSeparator(); CheckOverflow();
	
	if (type == PREPARE_REQUEST)
	{
		// <<<type specific>>> := <n>
		ulong64 proposalID;
		
		ReadNumber(proposalID);
		
		ValidateLength();
		PrepareRequest(paxosID, proposalID);
		return true;
	}
	else if (type == PREPARE_RESPONSE)
	{
		// <<<type specific>>> := <n>#<response>#<n_accepted>#<length>#<value>
		// the <n_accepted>#<length>#<value> is only present if
		// response == PREPARE_PREVIOUSLY_ACCEPTED
		ulong64	proposalID, acceptedProposalID;
		char	response;
		int		length;
		
		ReadNumber(proposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadChar(response);
	
		if (response != PREPARE_REJECTED &&
			response != PREPARE_PREVIOUSLY_ACCEPTED &&
			response != PREPARE_CURRENTLY_OPEN)
				return false;
		
		if (response == PREPARE_REJECTED || response == PREPARE_CURRENTLY_OPEN)
		{
			ValidateLength();
			PrepareResponse(paxosID, proposalID, response);
			return true;
		}

		CheckOverflow();		
		ReadSeparator(); CheckOverflow();
		ReadNumber(acceptedProposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();		
		ReadNumber(length); CheckOverflow();
		ReadSeparator();
		
		if (pos - data.buffer != data.length - length)
			return false;

		PrepareResponse(paxosID, proposalID, response, acceptedProposalID,
			ByteString(data.size - (pos - data.buffer), length, pos));
		return true;
	}
	else if (type == PROPOSE_REQUEST)
	{
		// <<<type specific>>> := <n>#<length>#<value>
		ulong64	proposalID;
		int		length;
		
		ReadNumber(proposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(length); CheckOverflow();
		ReadSeparator();
		
		if (pos - data.buffer != data.length - length)
			return false;
		
		ProposeRequest(paxosID, proposalID, ByteString(data.size - (pos - data.buffer),
			length, pos));
		return true;
	}
	else if (type == PROPOSE_RESPONSE)
	{
		// <<<type specific>>> := <n>#<response>
		ulong64	proposalID;
		char	response;
		
		ReadNumber(proposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadChar(response);
		
		if (response != PROPOSE_REJECTED &&
			response != PROPOSE_ACCEPTED)
				return false;
		
		ValidateLength();
		ProposeResponse(paxosID, proposalID, response);
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
	int required;
	
	if (type == PREPARE_REQUEST)
		required = snprintf(data.buffer, data.size, "%llu#%c#%llu", paxosID, type, proposalID);
	else if (type == PREPARE_RESPONSE)
	{
		if (response == PREPARE_REJECTED || response == PREPARE_CURRENTLY_OPEN)
			required = snprintf(data.buffer, data.size, "%llu#%c#%llu#%c", paxosID, type,
				proposalID, response);
		else
			required = snprintf(data.buffer, data.size, "%llu#%c#%llu#%c#%llu#%d#%*.s", paxosID,
				type, proposalID, response, acceptedProposalID, 
				value.length, value.length, value.buffer);
	}
	else if (type == PROPOSE_REQUEST)
		required = snprintf(data.buffer, data.size, "%llu#%c#%llu#%d#%.*s", paxosID, type,
			proposalID, value.length, value.length, value.buffer);
	else if (type == PROPOSE_RESPONSE)
		required = snprintf(data.buffer, data.size, "%llu#%c#%llu#%c", paxosID, type,
			proposalID, response);
	else if (type == LEARN_CHOSEN)
		required = snprintf(data.buffer, data.size, "%llu#%c#%d#%.*s", paxosID, type,
			value.length, value.length, value.buffer);
	else if (type == REQUEST_CHOSEN)
		required = snprintf(data.buffer, data.size, "%llu#%c", paxosID, type);
	else
		return false;
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;
}
