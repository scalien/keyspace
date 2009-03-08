#include "PLeaseLearner.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "PLeaseConsts.h"

PLeaseLearner::PLeaseLearner() :
	onRead(this, &PLeaseLearner::OnRead),
	onLeaseTimeout(this, &PLeaseLearner::OnLeaseTimeout),
	leaseTimeout(&onLeaseTimeout)
{
}

bool PLeaseLearner::Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_)
{
	// I/O framework
	ioproc = ioproc_;
	scheduler = scheduler_;
	config = config_;
	
	state.Init();
	
	if (!socket.Create(UDP)) return false;
	if (!socket.Bind(config->port + PLEASE_LEARNER_PORT_OFFSET)) return false;
	if (!socket.SetNonblocking()) return false;
	
	udpread.fd = socket.fd;
	udpread.data = rdata;
	udpread.onComplete = &onRead;

	return ioproc->Add(&udpread);
}

void PLeaseLearner::OnRead()
{
	Log_Trace();
		
	Log_Message("Participant %d received message %.*s from %s",
		config->nodeID, udpread.data.length, udpread.data.buffer, udpread.endpoint.ToString());
	
	if (!msg.Read(udpread.data))
	{
		Log_Message("PLeaseMsg::Read() failed: invalid message format");
		ioproc->Add(&udpread); // read again
	} else
		ProcessMsg();
	
	assert(udpread.active);
}

void PLeaseLearner::ProcessMsg()
{
	if (msg.type == LEARN_CHOSEN)
		OnLearnChosen();
	else
		ASSERT_FAIL();
}

void PLeaseLearner::OnLearnChosen()
{
	Log_Trace();
	
	if (msg.expireTime < Now())
	{
		state.learned = true;
		state.leaseOwner = msg.leaseOwner;
		state.expireTime = msg.expireTime;
		
		leaseTimeout.Set(state.expireTime);
		scheduler->Reset(&leaseTimeout);
		
		Call(onLearnLeaseCallback);
	}
	
	ioproc->Add(&udpread);
}

void PLeaseLearner::OnLeaseTimeout()
{
	state.OnLeaseTimeout();
	
	Call(onLeaseTimeoutCallback);
}

bool PLeaseLearner::IsLeaseOwner()
{
	if (state.learned && state.leaseOwner == config->nodeID && Now() < state.expireTime)
		return true;
	else
		return false;		
}

bool PLeaseLearner::LeaseKnown()
{
	if (state.learned && Now() < state.expireTime)
		return true;
	else
		return false;	
}

unsigned PLeaseLearner::LeaseOwner()
{
	return state.leaseOwner;
}

void PLeaseLearner::SetOnLearnLease(Callable* onLearnLeaseCallback_)
{
	onLearnLeaseCallback = onLearnLeaseCallback_;
}

void PLeaseLearner::SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback_)
{
	onLeaseTimeoutCallback = onLeaseTimeoutCallback_;
}
