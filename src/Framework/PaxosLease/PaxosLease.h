#ifndef PAXOSLEASE_H
#define PAXOSLEASE_H

#include "Framework/Transport/TransportReader.h"
#include "Framework/Transport/TransportWriter.h"
#include "Framework/Paxos/PaxosConfig.h"
#include "PLeaseMsg.h"
#include "PLeaseProposer.h"
#include "PLeaseAcceptor.h"
#include "PLeaseLearner.h"

class PaxosLease
{
public:
	PaxosLease();

	void				Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_);
	
	void				OnRead();
	
	void				AcquireLease();
	bool				IsLeaseOwner();
	bool				LeaseKnown();
	unsigned			LeaseOwner();
	ulong64				LeaseEpoch();
	
	void				SetOnLearnLease(Callable* onLearnLeaseCallback);
	void				SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback);
	
private:
	TransportReader*	reader;
	TransportWriter**	writers;
	
	MFunc<PaxosLease>	onRead;
	
	PLeaseMsg			msg;

 	PLeaseProposer		proposer;
	PLeaseAcceptor		acceptor;
	PLeaseLearner		learner;
};

#endif
