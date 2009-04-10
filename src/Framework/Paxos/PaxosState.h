#ifndef PAXOSSTATE_H
#define PAXOSSTATE_H

#include <math.h>
#include "System/Buffer.h"
#include <stdint.h>

class PaxosProposerState
{
public:
	bool					preparing;
	bool					proposing;
	
	uint64_t					proposalID;
	uint64_t					highestReceivedProposalID;
	ByteArray<VALUE_SIZE>	value;
	
	bool					leader;			// multi paxos
	unsigned				numProposals;	// number of proposal runs in this Paxos round
	
	bool Active() { return (preparing || proposing); }
	
	void Init()
	{
		preparing =					false;
		proposing =					false;
		proposalID =				0;
		highestReceivedProposalID =	0;
		value.Clear();
		leader = false;
		numProposals = 0;
	}	
};

class PaxosAcceptorState
{
public:
	uint64_t					promisedProposalID;
	bool					accepted;	
	uint64_t					acceptedProposalID;
	ByteArray<VALUE_SIZE>	acceptedValue;
	
	void Init()
	{
		promisedProposalID =	0;
		accepted =				false;
		acceptedProposalID =	0;
		acceptedValue.Clear();
	}
};

class PaxosLearnerState
{
public:
	bool					learned;
	ByteArray<VALUE_SIZE>	value;
	
	void Init()
	{
		learned =		0;
		value.Clear();
	}
};

#endif
