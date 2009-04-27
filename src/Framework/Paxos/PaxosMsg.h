#ifndef PAXOSMSG_H
#define PAXOSMSG_H

#include "PaxosConsts.h"
#include <unistd.h>
#include <string.h>
#include "System/Buffer.h"
#include <stdint.h>

// paxos message types:
#define PREPARE_REQUEST				'1'
#define	PREPARE_RESPONSE			'2'
#define PROPOSE_REQUEST				'3'
#define PROPOSE_RESPONSE			'4'
#define LEARN_CHOSEN				'5'
#define REQUEST_CHOSEN				'6'

// prepare responses:
#define PREPARE_REJECTED			'r'
#define PREPARE_PREVIOUSLY_ACCEPTED	'a'
#define PREPARE_CURRENTLY_OPEN		'o'

// propose responses:
#define PROPOSE_REJECTED			'r'
#define PROPOSE_ACCEPTED			'a'

// learn types
#define LEARN_PROPOSAL				'p'
#define LEARN_VALUE					'c'

class PaxosMsg
{
public:
	uint64_t				paxosID;
	unsigned				nodeID;
	char					type;
	uint64_t				proposalID;
	char					subtype;
	uint64_t				acceptedProposalID;
	uint64_t				promisedProposalID;
	ByteArray<PAXOS_VAL_SIZE>value;
	
	void					Init(uint64_t paxosID_, char type_, unsigned nodeID_);
		
	bool					PrepareRequest(uint64_t paxosID_, unsigned nodeID_, uint64_t proposalID_);
	bool					PrepareResponse(uint64_t paxosID_, unsigned nodeID_,
								uint64_t proposalID_, char subtype_);
	bool					PrepareResponse(uint64_t paxosID_, unsigned nodeID_,
								uint64_t proposalID_, char subtype_, uint64_t promisedProposalID_);
	bool					PrepareResponse(uint64_t paxosID_, unsigned nodeID_, uint64_t proposalID_,
								char subtype_, uint64_t acceptedProposalID_, ByteString value_);
	
	bool					ProposeRequest(uint64_t paxosID_, unsigned nodeID_, 
								uint64_t proposalID_, ByteString value_);
	bool					ProposeResponse(uint64_t paxosID_, unsigned nodeID_,
								uint64_t proposalID_, char subtype_);
	
	bool					LearnChosen(uint64_t paxosID_, unsigned nodeID_,
								char subtype_, ByteString value_);
	bool					LearnChosen(uint64_t paxosID_, unsigned nodeID_,
								char subtype_, uint64_t proposalID_);

	bool					RequestChosen(uint64_t paxosID_, unsigned nodeID_);

	bool					Read(ByteString& data);
	bool					Write(ByteString& data);
};

#endif
