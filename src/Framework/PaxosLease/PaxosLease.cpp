#include "PaxosLease.h"
#include "PLeaseConsts.h"
#include "System/Common.h"
#include "Framework/Transport/TransportUDPReader.h"
#include "Framework/Transport/TransportUDPWriter.h"

PaxosLease::PaxosLease()
: onRead(this, &PaxosLease::OnRead)
{
}

void PaxosLease::Init(IOProcessor* ioproc_, Scheduler* scheduler_, ReplicatedLog* replicatedLog_)
{
	InitTransport(ioproc_, scheduler_);
	
	proposer.Init(replicatedLog_, writers, scheduler_);
	acceptor.Init(replicatedLog_, writers, scheduler_);
	learner.Init(scheduler_);
}

void PaxosLease::InitTransport(IOProcessor* ioproc_, Scheduler* scheduler_)
{
	int			i;
	Endpoint	endpoint;
	
	reader = new TransportUDPReader;
	reader->Init(ioproc_, PaxosConfig::Get()->port + PLEASE_PORT_OFFSET);
	reader->SetOnRead(&onRead);
	
	writers = (TransportWriter**) Alloc(sizeof(TransportWriter*) * PaxosConfig::Get()->numNodes);
	for (i = 0; i < PaxosConfig::Get()->numNodes; i++)
	{
		endpoint = PaxosConfig::Get()->endpoints[i];
		endpoint.SetPort(endpoint.GetPort() + PLEASE_PORT_OFFSET);
		writers[i] = new TransportUDPWriter;
		writers[i]->Init(ioproc_, scheduler_, endpoint);
	}	
}

void PaxosLease::OnRead()
{
	ByteString bs;
	reader->GetMessage(bs);
	msg.Read(bs);
	
	if (msg.type == PREPARE_RESPONSE || msg.type == PROPOSE_RESPONSE)
		proposer.ProcessMsg(msg);
	else if (msg.type == PREPARE_REQUEST || msg.type == PROPOSE_REQUEST)
		acceptor.ProcessMsg(msg);
	else if (msg.type == LEARN_CHOSEN)
		learner.ProcessMsg(msg);
	else
		ASSERT_FAIL();
}

void PaxosLease::AcquireLease()
{
	proposer.AcquireLease();
}

bool PaxosLease::IsLeaseOwner()
{
	return learner.IsLeaseOwner();
}

bool PaxosLease::IsLeaseKnown()
{
	return learner.LeaseKnown();
}

unsigned PaxosLease::GetLeaseOwner()
{
	return learner.LeaseOwner();
}

ulong64 PaxosLease::GetLeaseEpoch()
{
	return learner.LeaseEpoch();
}

void PaxosLease::SetOnLearnLease(Callable* onLearnLeaseCallback)
{
	learner.SetOnLearnLease(onLearnLeaseCallback);
}

void PaxosLease::SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback)
{
	learner.SetOnLeaseTimeout(onLeaseTimeoutCallback);
}

void PaxosLease::Stop()
{
	reader->Stop();
}

void PaxosLease::Continue()
{
	reader->Continue();
}
