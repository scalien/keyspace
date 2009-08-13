#include "PLeaseLearner.h"
#include <inttypes.h>
#include "System/Log.h"
#include "System/Events/EventLoop.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
#include "PLeaseConsts.h"

PLeaseLearner::PLeaseLearner() :
	onLeaseTimeout(this, &PLeaseLearner::OnLeaseTimeout),
	leaseTimeout(&onLeaseTimeout)
{
}

void PLeaseLearner::Init(bool useSoftClock_)
{
	state.Init();
	useSoftClock = useSoftClock_;
}

void PLeaseLearner::ProcessMsg(PLeaseMsg &msg_)
{
	msg = msg_;

	if (msg.type == PLEASE_LEARN_CHOSEN)
		OnLearnChosen();
	else
		ASSERT_FAIL();
}

void PLeaseLearner::OnLearnChosen()
{
	Log_Trace();
	
	if (state.learned && state.expireTime < Now())
		OnLeaseTimeout();

	uint64_t expireTime;
	if (msg.leaseOwner == RCONF->GetNodeID())
		expireTime = msg.localExpireTime; // I'm the master
	else
		expireTime = Now() + msg.duration - 500 /* msec */;
		// conservative estimate
	
	if (expireTime < Now())
		return;
	
	if (!state.learned)
		Log_Message("PaxosLease: Node %d is the master", state.leaseOwner);
	
	state.learned = true;
	state.leaseOwner = msg.leaseOwner;
	state.expireTime = expireTime;
	Log_Trace("+++ Node %d has the lease for %" PRIu64 " msec +++",
		state.leaseOwner, state.expireTime - Now());

	leaseTimeout.Set(state.expireTime);
	EventLoop::Reset(&leaseTimeout);
	
	Call(onLearnLeaseCallback);
}

void PLeaseLearner::OnLeaseTimeout()
{
	if (state.learned)
		Log_Message("+++ Node %d lost its mastership +++", state.leaseOwner);

	EventLoop::Remove(&leaseTimeout);

	state.OnLeaseTimeout();
	
	Call(onLeaseTimeoutCallback);
}

void PLeaseLearner::CheckLease()
{
	uint64_t now;
	
	if (useSoftClock)
		now = EventLoop::Now();
	else
		now = Now();
	
	if (state.learned && state.expireTime < now)
		OnLeaseTimeout();
}

bool PLeaseLearner::IsLeaseOwner()
{
	CheckLease();
	
	if (state.learned && state.leaseOwner == RCONF->GetNodeID())
		return true;
	else
		return false;		
}

bool PLeaseLearner::IsLeaseKnown()
{
	CheckLease();
	
	if (state.learned)
		return true;
	else
		return false;	
}

int PLeaseLearner::GetLeaseOwner()
{
	CheckLease();
	
	if (state.learned)
		return state.leaseOwner;
	else
		return -1;
}

uint64_t PLeaseLearner::GetLeaseEpoch()
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
