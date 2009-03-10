#ifndef PAXOSSTATE_H
#define PAXOSSTATE_H

#include <math.h>
#include "System/Buffer.h"
#include "System/Types.h"

class PaxosProposerState
{
public:
	bool					preparing;
	bool					proposing;
	
	ulong64					proposalID;
	ulong64					highestReceivedProposalID;
	ByteArray<VALUE_SIZE>	value;
	
	bool					leader; // multi paxos
	
	bool Active() { return (preparing || proposing); }
	
	void Init()
	{
		preparing =					false;
		proposing =					false;
		proposalID =				0;
		highestReceivedProposalID =	0;
		
		value.Clear();
	}	
};

class PaxosAcceptorState
{
public:
	ulong64					promisedProposalID;
	
	bool					accepted;	
	ulong64					acceptedProposalID;
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
