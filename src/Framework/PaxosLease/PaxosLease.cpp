#include "PaxosLease.h"
#include "PLeaseConsts.h"

void PaxosLease::Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_)
{
	proposer.Init(ioproc_, scheduler_, config_);
	acceptor.Init(ioproc_, scheduler_, config_);
	learner.Init(ioproc_, scheduler_, config_);
}

void PaxosLease::AcquireLease()
{
	proposer.AcquireLease();
}

bool PaxosLease::IsLeaseOwner()
{
	return learner.IsLeaseOwner();
}

bool PaxosLease::LeaseKnown()
{
	return learner.LeaseKnown();
}

unsigned PaxosLease::LeaseOwner()
{
	return learner.LeaseOwner();
}

void PaxosLease::SetOnLearnLease(Callable* onLearnLeaseCallback)
{
	learner.SetOnLearnLease(onLearnLeaseCallback);
}

void PaxosLease::SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback)
{
	learner.SetOnLeaseTimeout(onLeaseTimeoutCallback);
}
