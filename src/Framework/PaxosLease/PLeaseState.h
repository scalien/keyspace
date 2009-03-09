#ifndef PLEASESTATE_H
#define PLEASESTATE_H

#include "System/Buffer.h"
#include "System/Types.h"

class PLeaseProposerState
{
public:
	bool					preparing;
	bool					proposing;
	
	ulong64					proposalID;
	ulong64					highestReceivedProposalID;

	unsigned				leaseOwner;
	ulong64					expireTime;
			
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
	ulong64					promisedProposalID;

	bool					accepted;
	ulong64					acceptedProposalID;
	unsigned				acceptedLeaseOwner;
	ulong64					acceptedExpireTime;
	
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
	int			leaseOwner;
	ulong64		expireTime;
	
	void Init()
	{
		learned =		0;
		leaseOwner =	0;
		expireTime =	0;
	}
	
	void OnLeaseTimeout()
	{
		learned =		0;
		leaseOwner =	0;
		expireTime =	0;
	}
};

#endif
