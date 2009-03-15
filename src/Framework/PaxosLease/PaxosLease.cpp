#include "PaxosLease.h"
#include "PLeaseConsts.h"
#include "System/Common.h"
#include "Framework/Transport/TransportUDPReader.h"
#include "Framework/Transport/TransportUDPWriter.h"

PaxosLease::PaxosLease()
: onRead(this, &PaxosLease::OnRead)
{
}

void PaxosLease::Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_)
{
	int			i;
	Endpoint	endpoint;
	Endpoint*	it;
	
	writers = (TransportWriter**) Alloc(sizeof(TransportWriter*) * config_->numNodes);
	it = config_->endpoints.Head();
	for (i = 0; i < config_->numNodes; i++)
	{
		endpoint = *it;
		endpoint.SetPort(endpoint.GetPort() + PLEASE_PORT_OFFSET);
		writers[i] = new TransportUDPWriter;
		writers[i]->Init(ioproc_, scheduler_, endpoint);
	
		it = config_->endpoints.Next(it);
	}
	
	reader = new TransportUDPReader;
	reader->Init(ioproc_, config_->port + PLEASE_PORT_OFFSET);
	reader->SetOnRead(&onRead);

	proposer.Init(writers, scheduler_, config_);
	acceptor.Init(writers, scheduler_, config_);
	learner.Init(scheduler_, config_);
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

bool PaxosLease::LeaseKnown()
{
	return learner.LeaseKnown();
}

unsigned PaxosLease::LeaseOwner()
{
	return learner.LeaseOwner();
}

ulong64 PaxosLease::LeaseEpoch()
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
