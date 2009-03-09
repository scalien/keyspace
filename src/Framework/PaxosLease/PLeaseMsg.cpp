#include "PLeaseMsg.h"
#include "System/Common.h"
#include <stdlib.h>
#include <stdio.h>

void PLeaseMsg::Init(char type_)
{
	type = type_;
}

bool PLeaseMsg::PrepareRequest(ulong64 proposalID_)
{
	Init(PREPARE_REQUEST);
	proposalID = proposalID_;

	return true;
}

bool PLeaseMsg::PrepareResponse(ulong64 proposalID_, char response_)
{
	Init(PREPARE_RESPONSE);
	proposalID = proposalID_;
	response = response_;

	return true;
}

bool PLeaseMsg::PrepareResponse(ulong64 proposalID_, char response_,
	ulong64 acceptedProposalID_, unsigned leaseOwner_,
								 ulong64 expireTime_)
{
	Init(PREPARE_RESPONSE);
	proposalID = proposalID_;
	response = response_;
	acceptedProposalID = acceptedProposalID_;
	leaseOwner = leaseOwner_;
	expireTime = expireTime_;
	
	return true;
}

bool PLeaseMsg::ProposeRequest(ulong64 proposalID_, unsigned int leaseOwner_,
													ulong64 expireTime_)
{
	Init(PROPOSE_REQUEST);
	proposalID = proposalID_;
	leaseOwner = leaseOwner_;
	expireTime = expireTime_;
	
	return true;
}

bool PLeaseMsg::ProposeResponse(ulong64 proposalID_, char response_)
{
	Init(PROPOSE_RESPONSE);
	proposalID = proposalID_;
	response = response_;

	return true;
}

bool PLeaseMsg::LearnChosen(unsigned leaseOwner_, ulong64 expireTime_)
{
	Init(LEARN_CHOSEN);
	leaseOwner = leaseOwner_;
	expireTime = expireTime_;

	return true;
}

bool PLeaseMsg::Read(ByteString& data)
{
	char		type;
	
	int			nread;
	char		*pos;
		
#define CheckOverflow()		if ((pos - data.buffer) >= data.length) return false;
#define ReadNumber(num)		(num) = strntoulong64(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadChar(c)			(c) = *pos; pos++;
#define ReadSeparator()		if (*pos != '#') return false; pos++;
#define ValidateLength()	if ((pos - data.buffer) != data.length) return false;

	pos = data.buffer;
	CheckOverflow();
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
		ulong64 proposalID;
		
		ReadNumber(proposalID);
		
		ValidateLength();
		PrepareRequest(proposalID);
		return true;
	}
	else if (type == PREPARE_RESPONSE)
	{
		// <<<type specific>>> := <n>#<response>#<n_accepted>#<length>#<value>
		// the <n_accepted>#<length>#<value> is only present if
		// response == PREPARE_PREVIOUSLY_ACCEPTED
		ulong64		proposalID, acceptedProposalID, expireTime;
		char		response;
		unsigned	leaseOwner;
		
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
			PrepareResponse(proposalID, response);
			return true;
		}

		CheckOverflow();		
		ReadSeparator(); CheckOverflow();
		ReadNumber(acceptedProposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(leaseOwner); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(expireTime);
		
		ValidateLength();
		PrepareResponse(proposalID, response, acceptedProposalID, leaseOwner, expireTime);
		return true;
	}
	else if (type == PROPOSE_REQUEST)
	{
		ulong64			proposalID, expireTime;
		unsigned int	leaseOwner;
		
		ReadNumber(proposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(leaseOwner); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(expireTime);
		
		ValidateLength();
		ProposeRequest(proposalID, leaseOwner, expireTime);
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
		ProposeResponse(proposalID, response);
		return true;
	}
	else if (type == LEARN_CHOSEN)
	{
		ReadNumber(leaseOwner); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(expireTime);
		
		ValidateLength();
		LearnChosen(leaseOwner, expireTime);
		return true;
	}
	
	return false;
}

bool PLeaseMsg::Write(ByteString& data)
{
	int required;
	
	if		(type == PREPARE_REQUEST)
	{
		required = snprintf(data.buffer, data.size, "%c#%llu", type, proposalID);
	}
	else if (type == PREPARE_RESPONSE)
	{
		if (response == PREPARE_REJECTED || response == PREPARE_CURRENTLY_OPEN)
			required = snprintf(data.buffer, data.size, "%c#%llu#%c", type, proposalID, response);
		else
			required = snprintf(data.buffer, data.size, "%c#%llu#%c#%llu#%d#%llu",
				type, proposalID, response, acceptedProposalID, leaseOwner, expireTime);
	}
	else if (type == PROPOSE_REQUEST)
	{
		required = snprintf(data.buffer, data.size, "%c#%llu#%d#%llu", type, proposalID,
			leaseOwner, expireTime);
	}
	else if (type == PROPOSE_RESPONSE)
	{
		required = snprintf(data.buffer, data.size, "%c#%llu#%c", type, proposalID, response);
	}
	else if (type == LEARN_CHOSEN)
	{
		required = snprintf(data.buffer, data.size, "%c#%d#%llu", type, leaseOwner, expireTime);
	}
	else
		ASSERT_FAIL();
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;
}
