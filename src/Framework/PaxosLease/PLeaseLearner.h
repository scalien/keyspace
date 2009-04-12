#ifndef PLEASELEARNER_H
#define PLEASELEARNER_H

#include "System/Common.h"
#include "System/Events/Timer.h"
#include "Framework/Transport/TransportWriter.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"

class PLeaseLearner
{
public:
	PLeaseLearner();
	
	void					Init();

	void					ProcessMsg(PLeaseMsg &msg);
	void					OnLeaseTimeout();
	bool					IsLeaseOwner();
	bool					LeaseKnown();
	int						LeaseOwner();
	uint64_t				LeaseEpoch();
	void					SetOnLearnLease(Callable* onLearnLeaseCallback_);
	void					SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback_);

private:
	void					OnLearnChosen();

	MFunc<PLeaseLearner>	onLeaseTimeout;
	Timer					leaseTimeout;
	PLeaseMsg				msg;	
	PLeaseLearnerState		state;	
	Callable*				onLearnLeaseCallback;
	Callable*				onLeaseTimeoutCallback;
};

#endif
