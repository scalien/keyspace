#include "PaxosLease.h"
#include "PLeaseConsts.h"
#include "System/Common.h"
#include "Framework/Transport/TransportUDPReader.h"
#include "Framework/Transport/TransportUDPWriter.h"

PaxosLease::PaxosLease()
: onRead(this, &PaxosLease::OnRead),
  onLearnLease(this, &PaxosLease::OnLearnLease),
  onLeaseTimeout(this, &PaxosLease::OnLeaseTimeout)
{
}

void PaxosLease::Init(bool useSoftClock)
{
	InitTransport();
	
	proposer.Init(writers);
	acceptor.Init(writers);
	learner.Init(useSoftClock);
	learner.SetOnLearnLease(&onLearnLease);
	learner.SetOnLeaseTimeout(&onLeaseTimeout);
	
	acquireLease = false;
}

void PaxosLease::InitTransport()
{
	unsigned	i;
	Endpoint	endpoint;
	
	reader = new TransportUDPReader;
	if (!reader->Init(RCONF->GetPort() + PLEASE_PORT_OFFSET))
		STOP_FAIL("cannot bind PaxosLease port", 1);
	reader->SetOnRead(&onRead);
	
	writers = (Writers)
	Alloc(sizeof(TransportUDPWriter*) * RCONF->GetNumNodes());
	
	for (i = 0; i < RCONF->GetNumNodes(); i++)
	{
		endpoint = RCONF->GetEndpoint(i);
		endpoint.SetPort(endpoint.GetPort() + PLEASE_PORT_OFFSET);
		writers[i] = new TransportUDPWriter;
		if (!writers[i]->Init(endpoint))
			STOP_FAIL("cannot bind PaxosLease port", 1);
	}	
}

void PaxosLease::OnRead()
{
	ByteString bs;
	reader->GetMessage(bs);
	if (!msg.Read(bs))
		return;	
	CheckNodeIdentity();
	
	if ((msg.IsRequest()) &&
		msg.proposalID > proposer.highestProposalID)
			proposer.highestProposalID = msg.proposalID;
	
	if (msg.IsResponse())
		proposer.ProcessMsg(msg);
	else if (msg.IsRequest())
		acceptor.ProcessMsg(msg);
	else if (msg.type == PLEASE_LEARN_CHOSEN)
		learner.ProcessMsg(msg);
	else
		ASSERT_FAIL();
}

void PaxosLease::AcquireLease()
{
	acquireLease = true;
	proposer.StartAcquireLease();
}

bool PaxosLease::IsLeaseOwner()
{
	return learner.IsLeaseOwner();
}

bool PaxosLease::IsLeaseKnown()
{
	return learner.IsLeaseKnown();
}

unsigned PaxosLease::GetLeaseOwner()
{
	return learner.GetLeaseOwner();
}

uint64_t PaxosLease::GetLeaseEpoch()
{
	return learner.GetLeaseEpoch();
}

void PaxosLease::SetOnLearnLease(Callable* onLearnLeaseCallback_)
{
	onLearnLeaseCallback = onLearnLeaseCallback_;
}

void PaxosLease::SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback_)
{
	onLeaseTimeoutCallback = onLeaseTimeoutCallback_;
}

void PaxosLease::Stop()
{
	reader->Stop();
}

void PaxosLease::Continue()
{
	reader->Continue();
}

bool PaxosLease::IsActive()
{
	return reader->IsActive();
}

void PaxosLease::OnNewPaxosRound()
{
	proposer.OnNewPaxosRound();
}

void PaxosLease::OnLearnLease()
{
	Log_Trace();
	
	Call(onLearnLeaseCallback);
	if (!IsLeaseOwner())
		proposer.StopAcquireLease();
}

void PaxosLease::OnLeaseTimeout()
{
	Log_Trace();
	
	Call(onLeaseTimeoutCallback);
	if (acquireLease)
		proposer.StartAcquireLease();
}

void PaxosLease::CheckNodeIdentity()
{
	Endpoint a, b;
	reader->GetEndpoint(a);
	
	b = RCONF->GetEndpoint(msg.nodeID);
	
	if (a.GetAddress() != ENDPOINT_ANY_ADDRESS && 
		b.GetAddress() != ENDPOINT_ANY_ADDRESS && 
		a.GetAddress() != b.GetAddress())
		STOP_FAIL("Node identity mismatch. Check all configuration files!", 0);
}
