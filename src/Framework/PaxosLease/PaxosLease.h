#ifndef PAXOSLEASE_H
#define PAXOSLEASE_H

#include "Framework/Transport/TransportReader.h"
#include "Framework/Transport/TransportWriter.h"
#include "Framework/Paxos/PaxosConfig.h"
#include "PLeaseConsts.h"
#include "PLeaseMsg.h"
#include "PLeaseProposer.h"
#include "PLeaseAcceptor.h"
#include "PLeaseLearner.h"

class ReplicatedLog;

class PaxosLease
{
public:
	PaxosLease();

	void				Init(IOProcessor* ioproc_, Scheduler* scheduler_,
							 ReplicatedLog* replicatedLog_, PaxosConfig* config_);
	void				OnRead();
	void				AcquireLease();
	bool				IsLeaseOwner();
	bool				IsLeaseKnown();
	unsigned			GetLeaseOwner();
	ulong64				GetLeaseEpoch();
	void				SetOnLearnLease(Callable* onLearnLeaseCallback);
	void				SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback);
	
private:
	void				InitTransport(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_);
	
	TransportReader*	reader;
	TransportWriter**	writers;
	MFunc<PaxosLease>	onRead;
	PLeaseMsg			msg;
 	PLeaseProposer		proposer;
	PLeaseAcceptor		acceptor;
	PLeaseLearner		learner;
};

#endif
