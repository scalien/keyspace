#include "PLeaseLearner.h"
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "System/Events/EventLoop.h"
#include "Framework/Paxos/PaxosConsts.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
#include "PLeaseConsts.h"

PLeaseLearner::PLeaseLearner() :
	onLeaseTimeout(this, &PLeaseLearner::OnLeaseTimeout),
	leaseTimeout(&onLeaseTimeout)
{
}

void PLeaseLearner::Init()
{
	state.Init();
}

void PLeaseLearner::ProcessMsg(PLeaseMsg &msg_)
{
	msg = msg_;

	if (msg.type == LEARN_CHOSEN)
		OnLearnChosen();
	else
		ASSERT_FAIL();
}

void PLeaseLearner::OnLearnChosen()
{
	Log_Trace();
	
	CheckLease();
	
	if (state.learned && state.leaseOwner != msg.leaseOwner)
		STOP_FAIL("Clock skew between nodes exceeds allowed maximum", 1);
				
	if (msg.expireTime > Now())
	{
		state.learned = true;
		state.leaseOwner = msg.leaseOwner;
		state.expireTime = msg.expireTime;
		
		Log_Message("+++ Node %d has the lease for %" PRIu64 " msec +++",
			state.leaseOwner, state.expireTime - Now());

		leaseTimeout.Set(state.expireTime);
		EventLoop::Reset(&leaseTimeout);
		
		Call(onLearnLeaseCallback);
	}
}

void PLeaseLearner::OnLeaseTimeout()
{
	EventLoop::Remove(&leaseTimeout);

	state.OnLeaseTimeout();
	
	Call(onLeaseTimeoutCallback);
}

bool PLeaseLearner::IsLeaseOwner()
{
	CheckLease();
	
	if (state.learned && state.leaseOwner == ReplicatedConfig::Get()->nodeID && Now() < state.expireTime)
		return true;
	else
		return false;		
}

bool PLeaseLearner::LeaseKnown()
{
	CheckLease();
	
	if (state.learned && Now() < state.expireTime)
		return true;
	else
		return false;	
}

int PLeaseLearner::LeaseOwner()
{
	CheckLease();
	
	if (state.learned && Now() < state.expireTime)
		return state.leaseOwner;
	else
		return -1;
}

uint64_t PLeaseLearner::LeaseEpoch()
{
	CheckLease();
	
	return state.leaseEpoch;
}

void PLeaseLearner::SetOnLearnLease(Callable* onLearnLeaseCallback_)
{
	onLearnLeaseCallback = onLearnLeaseCallback_;
}

void PLeaseLearner::SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback_)
{
	onLeaseTimeoutCallback = onLeaseTimeoutCallback_;
}

void PLeaseLearner::CheckLease()
{
	if (state.learned && state.expireTime < Now())
		OnLeaseTimeout();
}
