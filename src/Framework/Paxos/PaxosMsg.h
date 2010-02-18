#ifndef PAXOSMSG_H
#define PAXOSMSG_H

#include "System/Buffer.h"

#define PAXOS_PREPARE_REQUEST				'1'
#define PAXOS_PREPARE_REJECTED				'2'
#define PAXOS_PREPARE_PREVIOUSLY_ACCEPTED	'3'
#define PAXOS_PREPARE_CURRENTLY_OPEN		'4'
#define PAXOS_PROPOSE_REQUEST				'5'
#define PAXOS_PROPOSE_REJECTED				'6'
#define PAXOS_PROPOSE_ACCEPTED				'7'
#define PAXOS_LEARN_PROPOSAL				'8'
#define PAXOS_LEARN_VALUE					'9'
#define PAXOS_REQUEST_CHOSEN				'0'
#define PAXOS_START_CATCHUP					'c'

class PaxosMsg
{
public:
	uint64_t	paxosID;
	unsigned	nodeID;
	char		type;
	uint64_t	proposalID;
	uint64_t	acceptedProposalID;
	uint64_t	promisedProposalID;
	ByteString	value;

	void		Init(uint64_t paxosID_, char type_, unsigned nodeID_);
		
	bool		PrepareRequest(uint64_t paxosID_,
				unsigned nodeID_, uint64_t proposalID_);
	bool		PrepareRejected(uint64_t paxosID_, unsigned nodeID_,
				uint64_t proposalID_, uint64_t promisedProposalID_);
	bool		PreparePreviouslyAccepted(uint64_t paxosID_,
				unsigned nodeID_, uint64_t proposalID_,
				uint64_t acceptedProposalID_, ByteString value_);
	bool		PrepareCurrentlyOpen(uint64_t paxosID_,
				unsigned nodeID_, uint64_t proposalID_);	
	bool		ProposeRequest(uint64_t paxosID_, unsigned nodeID_, 
				uint64_t proposalID_, ByteString value_);
	bool		ProposeRejected(uint64_t paxosID_, unsigned nodeID_,
				uint64_t proposalID_);
	bool		ProposeAccepted(uint64_t paxosID_, unsigned nodeID_,
				uint64_t proposalID_);
	bool		LearnProposal(uint64_t paxosID_, unsigned nodeID_,
				uint64_t proposalID_);
	bool		LearnValue(uint64_t paxosID_, unsigned nodeID_,
				ByteString value_);
	bool		RequestChosen(uint64_t paxosID_, unsigned nodeID_);
	bool		StartCatchup(uint64_t paxosID_, unsigned nodeID_);

	bool		IsRequest();
	bool		IsPrepareResponse();
	bool		IsProposeResponse();
	bool		IsResponse();
	bool		IsLearn();

	bool		Read(const ByteString& data);
	bool		Write(ByteString& data);
};

#endif
