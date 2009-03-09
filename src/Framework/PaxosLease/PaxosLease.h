#ifndef PAXOSLEASE_H
#define PAXOSLEASE_H

#include "Framework/Paxos/PaxosConfig.h"
#include "PLeaseProposer.h"
#include "PLeaseAcceptor.h"
#include "PLeaseLearner.h"

class PaxosLease
{
public:
	void			Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_);
	
	void			AcquireLease();
	bool			IsLeaseOwner();
	bool			LeaseKnown();
	unsigned		LeaseOwner();
	
	void			SetOnLearnLease(Callable* onLearnLeaseCallback);
	void			SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback);

private:
 	PLeaseProposer	proposer;
	PLeaseAcceptor	acceptor;
	PLeaseLearner	learner;
};

#endif