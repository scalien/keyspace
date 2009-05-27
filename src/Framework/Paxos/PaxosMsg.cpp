#include "PaxosMsg.h"
#include "System/Common.h"

void PaxosMsg::Init(uint64_t paxosID_, char type_, unsigned nodeID_)
{
	paxosID = paxosID_;
	nodeID = nodeID_;
	type = type_;
}

bool PaxosMsg::PrepareRequest(uint64_t paxosID_,
unsigned nodeID_, uint64_t proposalID_)
{
	Init(paxosID_, PAXOS_PREPARE_REQUEST, nodeID_);
	proposalID = proposalID_;

	return true;
}

bool PaxosMsg::PrepareRejected(uint64_t paxosID_, unsigned nodeID_,
uint64_t proposalID_, uint64_t promisedProposalID_)
{
	Init(paxosID_, PAXOS_PREPARE_REJECTED, nodeID_);
	proposalID = proposalID_;
	promisedProposalID = promisedProposalID_;
	
	return true;
}


bool PaxosMsg::PreparePreviouslyAccepted(uint64_t paxosID_,
unsigned nodeID_, uint64_t proposalID_,
uint64_t acceptedProposalID_, ByteString value_)
{
	Init(paxosID_, PAXOS_PREPARE_PREVIOUSLY_ACCEPTED, nodeID_);
	proposalID = proposalID_;
	acceptedProposalID = acceptedProposalID_;
	value = value_;
	
	return true;
}

bool PaxosMsg::PrepareCurrentlyOpen(uint64_t paxosID_,
unsigned nodeID_, uint64_t proposalID_)
{
	Init(paxosID_, PAXOS_PREPARE_CURRENTLY_OPEN, nodeID_);
	proposalID = proposalID_;

	return true;
}

bool PaxosMsg::ProposeRequest(uint64_t paxosID_,
unsigned nodeID_, uint64_t proposalID_, ByteString value_)
{
	Init(paxosID_, PAXOS_PROPOSE_REQUEST, nodeID_);
	proposalID = proposalID_;
	value = value_;
	
	return true;
}

bool PaxosMsg::ProposeRejected(uint64_t paxosID_,
unsigned nodeID_, uint64_t proposalID_)
{
	Init(paxosID_, PAXOS_PROPOSE_REJECTED, nodeID_);
	proposalID = proposalID_;

	return true;
}

bool PaxosMsg::ProposeAccepted(uint64_t paxosID_,
unsigned nodeID_, uint64_t proposalID_)
{
	Init(paxosID_, PAXOS_PROPOSE_ACCEPTED, nodeID_);
	proposalID = proposalID_;

	return true;
}

bool PaxosMsg::LearnValue(uint64_t paxosID_,
unsigned nodeID_, ByteString value_)
{
	Init(paxosID_, PAXOS_LEARN_VALUE, nodeID_);
	value = value_;
	
	return true;
}

bool PaxosMsg::LearnProposal(uint64_t paxosID_,
unsigned nodeID_, uint64_t proposalID_)
{
	Init(paxosID_, PAXOS_LEARN_PROPOSAL, nodeID_);
	proposalID = proposalID_;
	
	return true;
}

bool PaxosMsg::RequestChosen(uint64_t paxosID_, unsigned nodeID_)
{
	Init(paxosID_, PAXOS_REQUEST_CHOSEN, nodeID_);
	
	return true;
}

bool PaxosMsg::IsRequest()
{
	return (type == PAXOS_PROPOSE_REQUEST ||
			type == PAXOS_PREPARE_REQUEST);
}

bool PaxosMsg::IsResponse()
{
	return IsPrepareResponse() || IsProposeResponse();
}

bool PaxosMsg::IsPrepareResponse()
{
	return (type == PAXOS_PREPARE_REJECTED ||
			type == PAXOS_PREPARE_PREVIOUSLY_ACCEPTED ||
			type == PAXOS_PREPARE_CURRENTLY_OPEN);
}

bool PaxosMsg::IsProposeResponse()
{
	return (type == PAXOS_PROPOSE_REJECTED ||
			type == PAXOS_PROPOSE_ACCEPTED);
}

bool PaxosMsg::IsLearn()
{
	return (type == PAXOS_LEARN_PROPOSAL ||
			type == PAXOS_LEARN_VALUE);
}

bool PaxosMsg::Read(const ByteString& data)
{
	int read;
	
	if (data.length < 1)
		return false;

	switch (data.buffer[0])
	{
		case PAXOS_PREPARE_REQUEST:
			read = snreadf(data.buffer, data.length, "%c:%U:%u:%U",
						   &type, &paxosID, &nodeID, &proposalID);
			break;
		case PAXOS_PREPARE_REJECTED:
			read = snreadf(data.buffer, data.length, "%c:%U:%u:%U:%U",
						   &type, &paxosID, &nodeID,
						   &proposalID, &promisedProposalID);
			break;
		case PAXOS_PREPARE_PREVIOUSLY_ACCEPTED:
			read = snreadf(data.buffer, data.length, "%c:%U:%u:%U:%U:%N",
						   &type, &paxosID, &nodeID, &proposalID,
						   &acceptedProposalID, &value);
			break;
		case PAXOS_PREPARE_CURRENTLY_OPEN:
			read = snreadf(data.buffer, data.length, "%c:%U:%u:%U",
						   &type, &paxosID, &nodeID, &proposalID);
			break;
		case PAXOS_PROPOSE_REQUEST:
			read = snreadf(data.buffer, data.length, "%c:%U:%u:%U:%N",
						   &type, &paxosID, &nodeID, &proposalID, &value);
			break;
		case PAXOS_PROPOSE_REJECTED:
			read = snreadf(data.buffer, data.length, "%c:%U:%u:%U",
						   &type, &paxosID, &nodeID, &proposalID);
			break;
		case PAXOS_PROPOSE_ACCEPTED:
			read = snreadf(data.buffer, data.length, "%c:%U:%u:%U",
						   &type, &paxosID, &nodeID, &proposalID);
			break;
		case PAXOS_LEARN_PROPOSAL:
			read = snreadf(data.buffer, data.length, "%c:%U:%u:%U",
						   &type, &paxosID, &nodeID, &proposalID);
			break;
		case PAXOS_LEARN_VALUE:
			read = snreadf(data.buffer, data.length, "%c:%U:%u:%N",
						   &type, &paxosID, &nodeID, &value);
			break;
		case PAXOS_REQUEST_CHOSEN:
			read = snreadf(data.buffer, data.length, "%c:%U:%u",
						   &type, &paxosID, &nodeID);
			break;
		default:
			return false;
	}
	
	return (read == (signed)data.length ? true : false);

}

bool PaxosMsg::Write(ByteString& data)
{
	switch (type)
	{
		case PAXOS_PREPARE_REQUEST:
			return data.Writef("%c:%U:%u:%U",
							   type, paxosID, nodeID, proposalID);
			break;
		case PAXOS_PREPARE_REJECTED:
			return data.Writef("%c:%U:%u:%U:%U",
						       type, paxosID, nodeID,
						       proposalID, promisedProposalID);
			break;
		case PAXOS_PREPARE_PREVIOUSLY_ACCEPTED:
			return data.Writef("%c:%U:%u:%U:%U:%M",
						       type, paxosID, nodeID, proposalID,
						       acceptedProposalID, &value);
			break;
		case PAXOS_PREPARE_CURRENTLY_OPEN:
			return data.Writef("%c:%U:%u:%U",
						       type, paxosID, nodeID, proposalID);
			break;
		case PAXOS_PROPOSE_REQUEST:
			return data.Writef("%c:%U:%u:%U:%M",
						       type, paxosID, nodeID, proposalID, &value);
			break;
		case PAXOS_PROPOSE_REJECTED:
			return data.Writef("%c:%U:%u:%U",
						       type, paxosID, nodeID, proposalID);
			break;
		case PAXOS_PROPOSE_ACCEPTED:
			return data.Writef("%c:%U:%u:%U",
						       type, paxosID, nodeID, proposalID);
			break;
		case PAXOS_LEARN_PROPOSAL:
			return data.Writef("%c:%U:%u:%U",
						       type, paxosID, nodeID, proposalID);
			break;
		case PAXOS_LEARN_VALUE:
			return data.Writef("%c:%U:%u:%M",
							   type, paxosID, nodeID, &value);
			break;
		case PAXOS_REQUEST_CHOSEN:
			return data.Writef("%c:%U:%u",
							   type, paxosID, nodeID);
			break;
		default:
			return false;
	}
}
