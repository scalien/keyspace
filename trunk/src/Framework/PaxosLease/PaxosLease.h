#ifndef PAXOSLEASE_H
#define PAXOSLEASE_H

#include "Framework/Transport/TransportUDPReader.h"
#include "Framework/Transport/TransportUDPWriter.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
#include "PLeaseConsts.h"
#include "PLeaseMsg.h"
#include "PLeaseProposer.h"
#include "PLeaseAcceptor.h"
#include "PLeaseLearner.h"

class ReplicatedLog;

class PaxosLease
{
	typedef TransportUDPReader*		Reader;
	typedef TransportUDPWriter**	Writers;
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
	void			CheckNodeIdentity();
	
private:
	void			InitTransport();
	
	bool			acquireLease;
	Reader			reader;
	Writers			writers;
	Func			onRead;
	Func			onLearnLease;
	Func			onLeaseTimeout;
	Callable*		onLearnLeaseCallback;
	Callable*		onLeaseTimeoutCallback;
	PLeaseMsg		msg;
 	PLeaseProposer	proposer;
	PLeaseAcceptor	acceptor;
	PLeaseLearner	learner;
};

#endif
