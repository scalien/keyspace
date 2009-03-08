#ifndef PLEASELEARNER_H
#define PLEASELEARNER_H

#include "System/Common.h"
#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"
#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "Framework/Database/Transaction.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"
#include "PaxosConfig.h"

class PLeaseLearner
{
public:
							PLeaseLearner();
	
	bool					Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_);
	
	void					OnRead();
	
	void					OnLeaseTimeout();
	
	bool					IsLeaseOwner();
	bool					LeaseKnown();
	unsigned				LeaseOwner();
	
	void					SetOnLearnLease(Callable* onLearnLeaseCallback_);
	void					SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback_);

private:
	void					ProcessMsg();
	virtual void			OnLearnChosen();

	IOProcessor*			ioproc;
	Scheduler*				scheduler;
	Socket					socket;

	UDPRead					udpread;
	
	ByteArray<64*KB>		rdata;
	
	MFunc<PLeaseLearner>	onRead;
	
	MFunc<PLeaseLearner>	onLeaseTimeout;
	Timer					leaseTimeout;
	
	PLeaseMsg				msg;
	
	PaxosConfig*			config;
	
	PLeaseLearnerState		state;
	
	Callable*				onLearnLeaseCallback;
	Callable*				onLeaseTimeoutCallback;
};

#endif