#include "PLeaseLearner.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "System/Events/EventLoop.h"
#include "Framework/Paxos/PaxosConsts.h"
#include "Framework/Paxos/PaxosConfig.h"
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
	
	if (msg.expireTime > Now())
	{
		state.learned = true;
		state.leaseOwner = msg.leaseOwner;
		state.expireTime = msg.expireTime;
		
		Log_Message("+++ Node %d has the lease for %llu msec +++",
			state.leaseOwner, state.expireTime - Now());

		leaseTimeout.Set(state.expireTime);
		EventLoop::Get()->Reset(&leaseTimeout);
		
		Call(onLearnLeaseCallback);
	}
}

void PLeaseLearner::OnLeaseTimeout()
{
	state.OnLeaseTimeout();
	
	Call(onLeaseTimeoutCallback);
}

bool PLeaseLearner::IsLeaseOwner()
{
	if (state.learned && state.leaseOwner == PaxosConfig::Get()->nodeID && Now() < state.expireTime)
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

ulong64 PLeaseLearner::LeaseEpoch()
{
	if (msg.expireTime < Now())
		state.OnLeaseTimeout();

	/*ulong64 left, middle, right, leaseEpoch;

	// <leaseEpoch since last restart> <restartCounter> <nodeID>
	
	left = state.leaseEpoch << (WIDTH_NODEID + WIDTH_RESTART_COUNTER);

	middle = config->restartCounter << WIDTH_NODEID;

	right = config->nodeID;

	leaseEpoch = left | middle | right;
	
	return leaseEpoch;*/
	
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
