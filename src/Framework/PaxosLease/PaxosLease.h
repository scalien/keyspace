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

	void				Init();
	void				OnRead();
	void				AcquireLease();
	bool				IsLeaseOwner();
	bool				IsLeaseKnown();
	unsigned			GetLeaseOwner();
	ulong64				GetLeaseEpoch();
	void				SetOnLearnLease(Callable* onLearnLeaseCallback);
	void				SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback);
	void				Stop();
	void				Continue();
	
private:
	void				InitTransport();
	
	TransportReader*	reader;
	TransportWriter**	writers;
	MFunc<PaxosLease>	onRead;
	PLeaseMsg			msg;
 	PLeaseProposer		proposer;
	PLeaseAcceptor		acceptor;
	PLeaseLearner		learner;
};

#endif
