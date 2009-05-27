#include "PLeaseMsg.h"

void PLeaseMsg::Init(char type_, unsigned nodeID_)
{
	type = type_;
	nodeID = nodeID_;
}

bool PLeaseMsg::PrepareRequest(unsigned nodeID_,
uint64_t proposalID_, uint64_t paxosID_)
{
	Init(PLEASE_PREPARE_REQUEST, nodeID_);
	proposalID = proposalID_;
	paxosID = paxosID_;
	
	return true;
}

bool PLeaseMsg::PrepareRejected(unsigned nodeID_, uint64_t proposalID_)
{
	Init(PLEASE_PREPARE_REJECTED, nodeID_);
	proposalID = proposalID_;

	return true;
}

bool PLeaseMsg::PreparePreviouslyAccepted(unsigned nodeID_,
uint64_t proposalID_, uint64_t acceptedProposalID_,
unsigned leaseOwner_, uint64_t expireTime_)
{
	Init(PLEASE_PREPARE_PREVIOUSLY_ACCEPTED, nodeID_);
	proposalID = proposalID_;
	acceptedProposalID = acceptedProposalID_;
	leaseOwner = leaseOwner_;
	expireTime = expireTime_;
	
	return true;
}

bool PLeaseMsg::PrepareCurrentlyOpen(unsigned nodeID_, uint64_t proposalID_)
{
	Init(PLEASE_PREPARE_CURRENTLY_OPEN, nodeID_);
	proposalID = proposalID_;

	return true;
}

bool PLeaseMsg::ProposeRequest(unsigned nodeID_,
uint64_t proposalID_, unsigned int leaseOwner_, uint64_t expireTime_)
{
	Init(PLEASE_PROPOSE_REQUEST, nodeID_);
	proposalID = proposalID_;
	leaseOwner = leaseOwner_;
	expireTime = expireTime_;
	
	return true;
}

bool PLeaseMsg::ProposeRejected(unsigned nodeID_, uint64_t proposalID_)
{
	Init(PLEASE_PROPOSE_REJECTED, nodeID_);
	proposalID = proposalID_;

	return true;
}

bool PLeaseMsg::ProposeAccepted(unsigned nodeID_, uint64_t proposalID_)
{
	Init(PLEASE_PROPOSE_ACCEPTED, nodeID_);
	proposalID = proposalID_;

	return true;
}

bool PLeaseMsg::LearnChosen(unsigned nodeID_,
unsigned leaseOwner_, uint64_t expireTime_)
{
	Init(PLEASE_LEARN_CHOSEN, nodeID_);
	leaseOwner = leaseOwner_;
	expireTime = expireTime_;

	return true;
}

bool PLeaseMsg::IsRequest()
{
	return (type == PLEASE_PROPOSE_REQUEST ||
			type == PLEASE_PREPARE_REQUEST);
}

bool PLeaseMsg::IsResponse()
{
	return IsPrepareResponse() || IsProposeResponse();
}

bool PLeaseMsg::IsPrepareResponse()
{
	return (type == PLEASE_PREPARE_REJECTED ||
			type == PLEASE_PREPARE_PREVIOUSLY_ACCEPTED ||
			type == PLEASE_PREPARE_CURRENTLY_OPEN);
}

bool PLeaseMsg::IsProposeResponse()
{
	return (type == PLEASE_PROPOSE_REJECTED ||
			type == PLEASE_PROPOSE_ACCEPTED);
}

bool PLeaseMsg::Read(const ByteString& data)
{
	int read;
	
	if (data.length < 1)
		return false;
	
	switch (data.buffer[0])
	{
		case PLEASE_PREPARE_REQUEST:
			read = snreadf(data.buffer, data.length, "%c:%u:%U:%U",
						   &type, &nodeID, &proposalID, &paxosID);
			break;
		case PLEASE_PREPARE_REJECTED:
			read = snreadf(data.buffer, data.length, "%c:%u:%U",
						   &type, &nodeID, &proposalID);
			break;
		case PLEASE_PREPARE_PREVIOUSLY_ACCEPTED:
			read = snreadf(data.buffer, data.length, "%c:%u:%U:%U:%u:%U",
						   &type, &nodeID, &proposalID, &acceptedProposalID,
						   &leaseOwner, &expireTime);
			break;
		case PLEASE_PREPARE_CURRENTLY_OPEN:
			read = snreadf(data.buffer, data.length, "%c:%u:%U",
						   &type, &nodeID, &proposalID);
			break;
		case PLEASE_PROPOSE_REQUEST:
			read = snreadf(data.buffer, data.length, "%c:%u:%U:%u:%U",
						   &type, &nodeID, &proposalID,
						   &leaseOwner, &expireTime);
			break;
		case PLEASE_PROPOSE_REJECTED:
			read = snreadf(data.buffer, data.length, "%c:%u:%U",
						   &type, &nodeID, &proposalID);
			break;
		case PLEASE_PROPOSE_ACCEPTED:
			read = snreadf(data.buffer, data.length, "%c:%u:%U",
						   &type, &nodeID, &proposalID);
			break;
		case PLEASE_LEARN_CHOSEN:
			read = snreadf(data.buffer, data.length, "%c:%u:%u:%U",
						   &type, &nodeID, &leaseOwner, &expireTime);
			break;
		default:
			return false;
	}

	return (read == (signed)data.length ? true : false);
}

bool PLeaseMsg::Write(ByteString& data)
{
	int req;
	
	switch (type)
	{
		case PLEASE_PREPARE_REQUEST:
			req = snwritef(data.buffer, data.size, "%c:%u:%U:%U",
						   type, nodeID, proposalID, paxosID);
			break;
		case PLEASE_PREPARE_REJECTED:
			req = snwritef(data.buffer, data.size, "%c:%u:%U",
						   type, nodeID, proposalID);
			break;
		case PLEASE_PREPARE_PREVIOUSLY_ACCEPTED:
			req = snwritef(data.buffer, data.size, "%c:%u:%U:%U:%u:%U",
						   type, nodeID, proposalID, acceptedProposalID,
						   leaseOwner, expireTime);
			break;
		case PLEASE_PREPARE_CURRENTLY_OPEN:
			req = snwritef(data.buffer, data.size, "%c:%u:%U",
						   type, nodeID, proposalID);
			break;
		case PLEASE_PROPOSE_REQUEST:
			req = snwritef(data.buffer, data.size, "%c:%u:%U:%u:%U",
						   type, nodeID, proposalID, leaseOwner, expireTime);
			break;
		case PLEASE_PROPOSE_REJECTED:
			req = snwritef(data.buffer, data.size, "%c:%u:%U",
						   type, nodeID, proposalID);
			break;
		case PLEASE_PROPOSE_ACCEPTED:
			req = snwritef(data.buffer, data.size, "%c:%u:%U",
						   type, nodeID, proposalID);
			break;
		case PLEASE_LEARN_CHOSEN:
			req = snwritef(data.buffer, data.size, "%c:%u:%u:%U",
						   type, nodeID, leaseOwner, expireTime);
			break;
		default:
			return false;
	}
	
	if (req < 0 || (unsigned)req > data.size)
		return false;
		
	data.length = req;
	return true;
}
