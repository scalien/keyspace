#ifndef PAXOSLEASE_H
#define PAXOSLEASE_H

#include "Framework/Transport/TransportTCPReader.h"
#include "Framework/Transport/TransportTCPWriter.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
#include "System/Events/Timer.h"
#include "PLeaseConsts.h"
#include "PLeaseMsg.h"
#include "PLeaseProposer.h"
#include "PLeaseAcceptor.h"
#include "PLeaseLearner.h"

class ReplicatedLog;

class PaxosLease
{
	typedef TransportTCPReader*		Reader;
	typedef TransportTCPWriter**	Writers;
	typedef MFunc<PaxosLease>		Func;
public:
	PaxosLease();

	void			Init(bool useSoftClock);
	void			OnRead();
	void			AcquireLease();
	bool			IsLeaseOwner();
	bool			IsLeaseKnown();
	unsigned		GetLeaseOwner();
	uint64_t		GetLeaseEpoch();
	void			SetOnLearnLease(Callable* onLearnLeaseCallback);
	void			SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback);
	void			Stop();
	void			Continue();
	bool			IsActive();
	void			OnNewPaxosRound();
	void			OnLearnLease();
	void			OnLeaseTimeout();
	void			OnStartupTimeout();
	
private:
	void			InitTransport();
	
	bool			acquireLease;
	Reader			reader;
	Writers			writers;
	Func			onRead;
	Func			onLearnLease;
	Func			onLeaseTimeout;
	Func			onStartupTimeout;
	CdownTimer		startupTimeout;
	Callable*		onLearnLeaseCallback;
	Callable*		onLeaseTimeoutCallback;
	PLeaseMsg		msg;
 	PLeaseProposer	proposer;
	PLeaseAcceptor	acceptor;
	PLeaseLearner	learner;
};

#endif
