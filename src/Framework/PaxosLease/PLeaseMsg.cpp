#include "PLeaseMsg.h"
#include "System/Common.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

void PLeaseMsg::Init(char type_, unsigned nodeID_)
{
	type = type_;
	nodeID = nodeID_;
}

bool PLeaseMsg::PrepareRequest(unsigned nodeID_, uint64_t proposalID_, uint64_t paxosID_)
{
	Init(PREPARE_REQUEST, nodeID_);
	proposalID = proposalID_;
	paxosID = paxosID_;
	
	return true;
}

bool PLeaseMsg::PrepareResponse(unsigned nodeID_, uint64_t proposalID_, char response_)
{
	Init(PREPARE_RESPONSE, nodeID_);
	proposalID = proposalID_;
	response = response_;

	return true;
}

bool PLeaseMsg::PrepareResponse(unsigned nodeID_, uint64_t proposalID_, char response_,
	uint64_t acceptedProposalID_, unsigned leaseOwner_, uint64_t expireTime_)
{
	Init(PREPARE_RESPONSE, nodeID_);
	proposalID = proposalID_;
	response = response_;
	acceptedProposalID = acceptedProposalID_;
	leaseOwner = leaseOwner_;
	expireTime = expireTime_;
	
	return true;
}

bool PLeaseMsg::ProposeRequest(unsigned nodeID_, uint64_t proposalID_, unsigned int leaseOwner_,
									uint64_t expireTime_)
{
	Init(PROPOSE_REQUEST, nodeID_);
	proposalID = proposalID_;
	leaseOwner = leaseOwner_;
	expireTime = expireTime_;
	
	return true;
}

bool PLeaseMsg::ProposeResponse(unsigned nodeID_, uint64_t proposalID_, char response_)
{
	Init(PROPOSE_RESPONSE, nodeID_);
	proposalID = proposalID_;
	response = response_;

	return true;
}

bool PLeaseMsg::LearnChosen(unsigned nodeID_, unsigned leaseOwner_, uint64_t expireTime_)
{
	Init(LEARN_CHOSEN, nodeID_);
	leaseOwner = leaseOwner_;
	expireTime = expireTime_;

	return true;
}

bool PLeaseMsg::Read(ByteString& data)
{
	char		type;
	
	unsigned	nread;
	char		*pos;
		
#define CheckOverflow()		if ((pos - data.buffer) >= (int) data.length || pos < data.buffer) return false;
#define ReadUint64(num)		(num) = strntouint64(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadChar(c)			(c) = *pos; pos++;
#define ReadSeparator()		if (*pos != ':') return false; pos++;
#define ValidateLength()	if ((pos - data.buffer) != (int)data.length) return false;

	pos = data.buffer;
	CheckOverflow();
	ReadChar(type);	CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadUint64(nodeID); CheckOverflow();
	
	if (type != PREPARE_REQUEST	&&
		type != PREPARE_RESPONSE &&
		type != PROPOSE_REQUEST &&
		type != PROPOSE_RESPONSE &&
		type != LEARN_CHOSEN)
			return false;
	
	ReadSeparator(); CheckOverflow();
	
	if (type == PREPARE_REQUEST)
	{
		
		ReadUint64(proposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64(paxosID);
		
		ValidateLength();
		PrepareRequest(nodeID, proposalID, paxosID);
		return true;
	}
	else if (type == PREPARE_RESPONSE)
	{
		ReadUint64(proposalID); CheckOverflow();
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
		ReadUint64(acceptedProposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64(leaseOwner); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64(expireTime);
		
		ValidateLength();
		PrepareResponse(nodeID, proposalID, response, acceptedProposalID, leaseOwner, expireTime);
		return true;
	}
	else if (type == PROPOSE_REQUEST)
	{		
		ReadUint64(proposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64(leaseOwner); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64(expireTime);
		
		ValidateLength();
		ProposeRequest(nodeID, proposalID, leaseOwner, expireTime);
		return true;
	}
	else if (type == PROPOSE_RESPONSE)
	{
		ReadUint64(proposalID); CheckOverflow();
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
		ReadUint64(leaseOwner); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64(expireTime);
		
		ValidateLength();
		LearnChosen(nodeID, leaseOwner, expireTime);
		return true;
	}
	
	return false;
}

bool PLeaseMsg::Write(ByteString& data)
{
	unsigned required;
	
	if		(type == PREPARE_REQUEST)
	{
		required = snwritef(data.buffer, data.size, "%c:%d:%U:%U", type, nodeID, proposalID, paxosID);
	}
	else if (type == PREPARE_RESPONSE)
	{
		if (response == PREPARE_REJECTED || response == PREPARE_CURRENTLY_OPEN)
			required = snwritef(data.buffer, data.size, "%c:%d:%U:%c", type, nodeID,
				proposalID, response);
		else
			required = snwritef(data.buffer, data.size, "%c:%d:%U:%c:%U:%d:%U",
				type, nodeID, proposalID, response, acceptedProposalID, leaseOwner, expireTime);
	}
	else if (type == PROPOSE_REQUEST)
	{
		required = snwritef(data.buffer, data.size, "%c:%d:%U:%d:%U", type, nodeID, proposalID,
			leaseOwner, expireTime);
	}
	else if (type == PROPOSE_RESPONSE)
	{
		required = snwritef(data.buffer, data.size, "%c:%d:%U:%c", type, nodeID, proposalID, response);
	}
	else if (type == LEARN_CHOSEN)
	{
		required = snwritef(data.buffer, data.size, "%c:%d:%d:%U", type, nodeID, leaseOwner, expireTime);
	}
	else
		ASSERT_FAIL();
	
	if (required < 0 || (unsigned)required > data.size)
		return false;
		
	data.length = required;
	return true;
}
