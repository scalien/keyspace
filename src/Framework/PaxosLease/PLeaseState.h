#ifndef PLEASESTATE_H
#define PLEASESTATE_H

#include "System/Buffer.h"
#include <stdint.h>

class PLeaseProposerState
{
public:
	bool		preparing;
	bool		proposing;
	
	uint64_t	proposalID;
	uint64_t	highestReceivedProposalID;

	unsigned	leaseOwner;
	uint64_t	expireTime;
			
	bool Active() { return (preparing || proposing); }
	
	void Init()
	{
		preparing =					false;
		proposing =					false;
		proposalID =				0;
		highestReceivedProposalID =	0;
	
		expireTime =				0;
	}
};

class PLeaseAcceptorState
{
public:
	uint64_t	promisedProposalID;

	bool		accepted;
	uint64_t	acceptedProposalID;
	unsigned	acceptedLeaseOwner;
	uint64_t	acceptedExpireTime;
	
	void Init()
	{
		promisedProposalID =		0;

		accepted =				false;
		acceptedProposalID =		0;
		acceptedLeaseOwner =		0;
		acceptedExpireTime =		0;
	}

	void OnLeaseTimeout()
	{
		accepted =				false;
		acceptedProposalID =		0;
		acceptedLeaseOwner =		0;
		acceptedExpireTime =		0;
	}
};

class PLeaseLearnerState
{
public:
	bool		learned;
	unsigned	leaseOwner;
	uint64_t	expireTime;
	uint64_t	leaseEpoch;
	
	void Init()
	{
		learned =		0;
		leaseOwner =	0;
		expireTime =	0;
		leaseEpoch =	0;
	}
	
	void OnLeaseTimeout()
	{
		learned =		0;
		leaseOwner =	0;
		expireTime =	0;
		leaseEpoch++;
	}
};

#endif
