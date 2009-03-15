#ifndef PAXOSMSG_H
#define PAXOSMSG_H

#include "PaxosConsts.h"
#include <unistd.h>
#include <string.h>
#include "System/Buffer.h"
#include "System/Types.h"

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
	ulong64					paxosID;
	unsigned				nodeID;
	char					type;
	ulong64					proposalID;
	char					subtype;
	ulong64					acceptedProposalID;
	ByteArray<VALUE_SIZE>	value;
	
	void					Init(ulong64 paxosID_, char type_, unsigned nodeID_);
		
	bool					PrepareRequest(ulong64 paxosID_, unsigned nodeID_, ulong64 proposalID_);
	bool					PrepareResponse(ulong64 paxosID_, unsigned nodeID_,
								ulong64 proposalID_, char subtype_);
	bool					PrepareResponse(ulong64 paxosID_, unsigned nodeID_, ulong64 proposalID_,
								char subtype_, ulong64 acceptedProposalID_, ByteString value_);
	
	bool					ProposeRequest(ulong64 paxosID_, unsigned nodeID_, 
								ulong64 proposalID_, ByteString value_);
	bool					ProposeResponse(ulong64 paxosID_, unsigned nodeID_,
								ulong64 proposalID_, char subtype_);
	
	bool					LearnChosen(ulong64 paxosID_, unsigned nodeID_,
								char subtype_, ByteString value_);
	bool					LearnChosen(ulong64 paxosID_, unsigned nodeID_,
								char subtype_, ulong64 proposalID_);

	bool					RequestChosen(ulong64 paxosID_, unsigned nodeID_);

	bool					Read(ByteString& data);
	bool					Write(ByteString& data);
};

#endif
