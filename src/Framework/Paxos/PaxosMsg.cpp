#include "PaxosMsg.h"
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include "System/Common.h"

void PaxosMsg::Init(uint64_t paxosID_, char type_, unsigned nodeID_)
{
	paxosID = paxosID_;
	nodeID = nodeID_;
	type = type_;
}

bool PaxosMsg::PrepareRequest(uint64_t paxosID_, unsigned nodeID_, uint64_t proposalID_)
{
	Init(paxosID_, PREPARE_REQUEST, nodeID_);
	proposalID = proposalID_;

	return true;
}

bool PaxosMsg::PrepareResponse(uint64_t paxosID_, unsigned nodeID_, uint64_t proposalID_, char subtype_)
{
	Init(paxosID_, PREPARE_RESPONSE, nodeID_);
	proposalID = proposalID_;
	subtype = subtype_;

	return true;
}

bool PaxosMsg::PrepareResponse(uint64_t paxosID_, unsigned nodeID_, uint64_t proposalID_, char subtype_,
	uint64_t promisedProposalID_)
{
	Init(paxosID_, PREPARE_RESPONSE, nodeID_);
	proposalID = proposalID_;
	subtype = subtype_;
	promisedProposalID = promisedProposalID_;
	
	return true;
}


bool PaxosMsg::PrepareResponse(uint64_t paxosID_, unsigned nodeID_, uint64_t proposalID_, char subtype_,
	uint64_t acceptedProposalID_, ByteString value_)
{
	Init(paxosID_, PREPARE_RESPONSE, nodeID_);
	proposalID = proposalID_;
	subtype = subtype_;
	acceptedProposalID = acceptedProposalID_;
	if (!value.Set(value_))
		return false;
	
	return true;
}

bool PaxosMsg::ProposeRequest(uint64_t paxosID_, unsigned nodeID_, uint64_t proposalID_, ByteString value_)
{
	Init(paxosID_, PROPOSE_REQUEST, nodeID_);
	proposalID = proposalID_;
	if (!value.Set(value_))
		return false;
	
	return true;
}

bool PaxosMsg::ProposeResponse(uint64_t paxosID_, unsigned nodeID_, uint64_t proposalID_, char subtype_)
{
	Init(paxosID_, PROPOSE_RESPONSE, nodeID_);
	proposalID = proposalID_;
	subtype = subtype_;

	return true;
}

bool PaxosMsg::LearnChosen(uint64_t paxosID_, unsigned nodeID_, char subtype_, ByteString value_)
{
	Init(paxosID_, LEARN_CHOSEN, nodeID_);
	subtype = subtype_;
	if (!value.Set(value_))
		return false;
	
	return true;
}

bool PaxosMsg::LearnChosen(uint64_t paxosID_, unsigned nodeID_, char subtype_, uint64_t proposalID_)
{
	Init(paxosID_, LEARN_CHOSEN, nodeID_);
	subtype = subtype_;
	proposalID = proposalID_;
	
	return true;
}

bool PaxosMsg::RequestChosen(uint64_t paxosID_, unsigned nodeID_)
{
	Init(paxosID_, REQUEST_CHOSEN, nodeID_);
	
	return true;
}

bool PaxosMsg::Read(ByteString& data)
{
	uint64_t	paxosID;
	char		type;
	
	unsigned	nread;
	char		*pos;
		
#define CheckOverflow()		if ((pos - data.buffer) >= (int) data.length || pos < data.buffer) return false;
#define ReadUint64_t(num)		(num) = strntouint64_t(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadChar(c)			(c) = *pos; pos++;
#define ReadSeparator()		if (*pos != '#') return false; pos++;
#define ValidateLength()	if ((pos - data.buffer) != (int) data.length) return false;

	pos = data.buffer;
	CheckOverflow();
	ReadUint64_t(paxosID); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadChar(type); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadUint64_t(nodeID);
		
	if (type == REQUEST_CHOSEN)
	{
		ValidateLength();
		RequestChosen(paxosID, nodeID);
		return true;
	}
	
	CheckOverflow();
	ReadSeparator(); CheckOverflow();
	
	if (type == PREPARE_REQUEST)
	{
		uint64_t proposalID;
		
		ReadUint64_t(proposalID);
		
		ValidateLength();
		PrepareRequest(paxosID, nodeID, proposalID);
		return true;
	}
	else if (type == PREPARE_RESPONSE)
	{
		uint64_t	proposalID, acceptedProposalID;
		char		response;
		unsigned	length;
		
		ReadUint64_t(proposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadChar(response);
	
		if (response != PREPARE_REJECTED &&
			response != PREPARE_PREVIOUSLY_ACCEPTED &&
			response != PREPARE_CURRENTLY_OPEN)
				return false;

		if (response == PREPARE_REJECTED)
		{
			CheckOverflow();
			ReadSeparator(); CheckOverflow();
			ReadUint64_t(promisedProposalID);
			ValidateLength();
			PrepareResponse(paxosID, nodeID, proposalID, response, promisedProposalID);
			return true;
		}

		
		if (response == PREPARE_CURRENTLY_OPEN)
		{
			ValidateLength();
			PrepareResponse(paxosID, nodeID, proposalID, response);
			return true;
		}

		CheckOverflow();		
		ReadSeparator(); CheckOverflow();
		ReadUint64_t(acceptedProposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();		
		ReadUint64_t(length); CheckOverflow();
		ReadSeparator();
		
		if (pos - data.buffer != (int)(data.length - length))
			return false;

		PrepareResponse(paxosID, nodeID, proposalID, response, acceptedProposalID,
			ByteString(data.size - (pos - data.buffer), length, pos));
		return true;
	}
	else if (type == PROPOSE_REQUEST)
	{
		uint64_t	proposalID;
		unsigned	length;
		
		ReadUint64_t(proposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64_t(length); CheckOverflow();
		ReadSeparator();
		
		if (pos - data.buffer != (int)(data.length - length))
			return false;
		
		ProposeRequest(paxosID, nodeID, proposalID, ByteString(data.size - (pos - data.buffer),
			length, pos));
		return true;
	}
	else if (type == PROPOSE_RESPONSE)
	{
		uint64_t	proposalID;
		char		response;
		
		ReadUint64_t(proposalID); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadChar(response);
		
		if (response != PROPOSE_REJECTED &&
			response != PROPOSE_ACCEPTED)
				return false;
		
		ValidateLength();
		ProposeResponse(paxosID, nodeID, proposalID, response);
		return true;
	}
	else if (type == LEARN_CHOSEN)
	{
		unsigned		length;
		
		ReadChar(subtype); CheckOverflow();
		ReadSeparator(); CheckOverflow();
		
		if (subtype == LEARN_VALUE)
		{
			ReadUint64_t(length); CheckOverflow();
			ReadSeparator();
			
			if (pos - data.buffer != (int)(data.length - length))
				return false;
			
			LearnChosen(paxosID, nodeID, LEARN_VALUE,
				ByteString(data.size - (pos - data.buffer), length, pos));
		}
		else
		{
			ReadUint64_t(proposalID);
			
			ValidateLength();
			LearnChosen(paxosID, nodeID, LEARN_PROPOSAL, proposalID);
			return true;

		}
		return true;
	}
	
	return false;
}

bool PaxosMsg::Write(ByteString& data)
{
	int required;
	
	if (type == PREPARE_REQUEST)
		required = snwritef(data.buffer, data.size, "%U#%c#%d#%U", paxosID, type, nodeID, proposalID);
	else if (type == PREPARE_RESPONSE)
	{
		if (subtype == PREPARE_REJECTED)
		{
			required = snwritef(data.buffer, data.size, "%U#%c#%d#%U#%c#%U",
				paxosID, type, nodeID, proposalID, subtype, promisedProposalID);
		}
		else if (subtype == PREPARE_CURRENTLY_OPEN)
			required = snwritef(data.buffer, data.size, "%U#%c#%d#%U#%c",
				paxosID, type, nodeID, proposalID, subtype);
		else
			required = snwritef(data.buffer, data.size, "%U#%c#%d#%U#%c#%U#%d#%B", paxosID,
				type, nodeID, proposalID, subtype, acceptedProposalID, 
				value.length, value.length, value.buffer);
	}
	else if (type == PROPOSE_REQUEST)
		required = snwritef(data.buffer, data.size, "%U#%c#%d#%U#%d#%B",
			paxosID, type, nodeID, proposalID, value.length, value.length, value.buffer);
	else if (type == PROPOSE_RESPONSE)
		required = snwritef(data.buffer, data.size, "%U#%c#%d#%U#%c",
			paxosID, type, nodeID, proposalID, subtype);
	else if (type == LEARN_CHOSEN)
	{
		if (subtype == LEARN_VALUE)
			required = snwritef(data.buffer, data.size, "%U#%c#%d#%c#%d#%B",
				paxosID, type, nodeID, subtype, value.length, value.length, value.buffer);
		else
			required = snwritef(data.buffer, data.size, "%U#%c#%d#%c#%U",
				paxosID, type, nodeID, subtype, proposalID);
	}
	else if (type == REQUEST_CHOSEN)
		required = snwritef(data.buffer, data.size, "%U#%c#%d", paxosID, type, nodeID);
	else
		return false;
	
	if (required < 0 || (unsigned)required > data.size)
		return false;
		
	data.length = required;
	return true;
}
