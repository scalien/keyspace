#ifndef PAXOSLEASE_H
#define PAXOSLEASE_H

#include "Framework/Transport/TransportReader.h"
#include "Framework/Transport/TransportWriter.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
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
	uint64_t			GetLeaseEpoch();
	void				SetOnLearnLease(Callable* onLearnLeaseCallback);
	void				SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback);
	void				Stop();
	void				Continue();
	bool				IsActive();
	void				OnNewPaxosRound();
	void				OnLearnLease();
	void				OnLeaseTimeout();
	
private:
	void				InitTransport();
	
	bool				acquireLease;
	TransportReader*	reader;
	TransportWriter**	writers;
	MFunc<PaxosLease>	onRead;
	MFunc<PaxosLease>	onLearnLease;
	MFunc<PaxosLease>	onLeaseTimeout;
	Callable*			onLearnLeaseCallback;
	Callable*			onLeaseTimeoutCallback;
	PLeaseMsg			msg;
 	PLeaseProposer		proposer;
	PLeaseAcceptor		acceptor;
	PLeaseLearner		learner;
};

#endif
