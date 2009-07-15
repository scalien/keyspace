#ifndef PLEASELEARNER_H
#define PLEASELEARNER_H

#include "System/Common.h"
#include "System/Events/Timer.h"
#include "Framework/Transport/TransportUDPWriter.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"

class PLeaseLearner
{
	typedef MFunc<PLeaseLearner>	Func;
	typedef PLeaseLearnerState		State;
	
public:
	PLeaseLearner();
	
	void		Init(bool useSoftClock_);

	void		ProcessMsg(PLeaseMsg &msg);
	void		OnLeaseTimeout();
	bool		IsLeaseOwner();
	bool		IsLeaseKnown();
	int			GetLeaseOwner();
	uint64_t	GetLeaseEpoch();
	void		SetOnLearnLease(Callable* onLearnLeaseCallback_);
	void		SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback_);

private:
	void		OnLearnChosen();
	void		CheckLease();

	Func		onLeaseTimeout;
	Timer		leaseTimeout;
	PLeaseMsg	msg;	
	State		state;	
	Callable*	onLearnLeaseCallback;
	Callable*	onLeaseTimeoutCallback;
	bool		useSoftClock;
};

#endif
