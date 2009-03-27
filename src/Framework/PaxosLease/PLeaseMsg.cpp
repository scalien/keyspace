#include "PLeaseMsg.h"
#include "System/Common.h"
#include <stdlib.h>
#include <stdio.h>

void PLeaseMsg::Init(char type_, unsigned nodeID_)
{
	type = type_;
	nodeID = nodeID_;
}

bool PLeaseMsg::PrepareRequest(unsigned nodeID_, ulong64 proposalID_, ulong64 paxosID_)
{
	Init(PREPARE_REQUEST, nodeID_);
	proposalID = proposalID_;
	paxosID = paxosID_;
	
	return true;
}

bool PLeaseMsg::PrepareResponse(unsigned nodeID_, ulong64 proposalID_, char response_)
{
	Init(PREPARE_RESPONSE, nodeID_);
	proposalID = proposalID_;
	response = response_;

	return true;
}

bool PLeaseMsg::PrepareResponse(unsigned nodeID_, ulong64 proposalID_, char response_,
	ulong64 acceptedProposalID_, unsigned leaseOwner_, ulong64 expireTime_)
{
	Init(PREPARE_RESPONSE, nodeID_);
	proposalID = proposalID_;
	response = response_;
	acceptedProposalID = acceptedProposalID_;
	leaseOwner = leaseOwner_;
	expireTime = expireTime_;
	
	return true;
}

bool PLeaseMsg::ProposeRequest(unsigned nodeID_, ulong64 proposalID_, unsigned int leaseOwner_,
									ulong64 expireTime_)
{
	Init(PROPOSE_REQUEST, nodeID_);
	proposalID = proposalID_;
	leaseOwner = leaseOwner_;
	expireTime = expireTime_;
	
	return true;
}

bool PLeaseMsg::ProposeResponse(unsigned nodeID_, ulong64 proposalID_, char response_)
{
	Init(PROPOSE_RESPONSE, nodeID_);
	proposalID = proposalID_;
	response = response_;

	return true;
}

bool PLeaseMsg::LearnChosen(unsigned nodeID_, unsigned leaseOwner_, ulong64 expireTime_)
{
	Init(LEARN_CHOSEN, nodeID_);
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
	ReadSeparator(); CheckOverflow();
	ReadNumber(nodeID); CheckOverflow();
	
	if (type != PREPARE_REQUEST	&&
		type != PREPARE_RESPONSE &&
		type != PROPOSE_REQUEST &&
		type != PROPOSE_RESPONSE &&
		type != LEARN_CHOSEN)
			return false;
	
	ReadSeparator(); CheckOverflow();
	
	if (type == PREPARE_REQUEST)
	{
		
		ReadNumber(proposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(paxosID);
		
		ValidateLength();
		PrepareRequest(nodeID, proposalID, paxosID);
		return true;
	}
	else if (type == PREPARE_RESPONSE)
	{
		// <<<type specific>>> := <n>#<response>#<n_accepted>#<length>#<value>
		// the <n_accepted>#<length>#<value> is only present if
		// response == PREPARE_PREVIOUSLY_ACCEPTED
		
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
			PrepareResponse(nodeID, proposalID, response);
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
		PrepareResponse(nodeID, proposalID, response, acceptedProposalID, leaseOwner, expireTime);
		return true;
	}
	else if (type == PROPOSE_REQUEST)
	{		
		ReadNumber(proposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(leaseOwner); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(expireTime);
		
		ValidateLength();
		ProposeRequest(nodeID, proposalID, leaseOwner, expireTime);
		return true;
	}
	else if (type == PROPOSE_RESPONSE)
	{
		// <<<type specific>>> := <n>#<response>

		ReadNumber(proposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadChar(response);
		
		if (response != PROPOSE_REJECTED &&
			response != PROPOSE_ACCEPTED)
				return false;
		
		ValidateLength();
		ProposeResponse(nodeID, proposalID, response);
		return true;
	}
	else if (type == LEARN_CHOSEN)
	{
		ReadNumber(leaseOwner); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadNumber(expireTime);
		
		ValidateLength();
		LearnChosen(nodeID, leaseOwner, expireTime);
		return true;
	}
	
	return false;
}

bool PLeaseMsg::Write(ByteString& data)
{
	int required;
	
	if		(type == PREPARE_REQUEST)
	{
		required = snprintf(data.buffer, data.size, "%c#%d#%llu#%llu", type, nodeID, proposalID, paxosID);
	}
	else if (type == PREPARE_RESPONSE)
	{
		if (response == PREPARE_REJECTED || response == PREPARE_CURRENTLY_OPEN)
			required = snprintf(data.buffer, data.size, "%c#%d#%llu#%c", type, nodeID,
				proposalID, response);
		else
			required = snprintf(data.buffer, data.size, "%c#%d#%llu#%c#%llu#%d#%llu",
				type, nodeID, proposalID, response, acceptedProposalID, leaseOwner, expireTime);
	}
	else if (type == PROPOSE_REQUEST)
	{
		required = snprintf(data.buffer, data.size, "%c#%d#%llu#%d#%llu", type, nodeID, proposalID,
			leaseOwner, expireTime);
	}
	else if (type == PROPOSE_RESPONSE)
	{
		required = snprintf(data.buffer, data.size, "%c#%d#%llu#%c", type, nodeID, proposalID, response);
	}
	else if (type == LEARN_CHOSEN)
	{
		required = snprintf(data.buffer, data.size, "%c#%d#%d#%llu", type, nodeID, leaseOwner, expireTime);
	}
	else
		ASSERT_FAIL();
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;
}
