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

void PaxosLease::Init()
{
	InitTransport();
	
	proposer.Init(writers);
	acceptor.Init(writers);
	learner.Init();
	learner.SetOnLearnLease(&onLearnLease);
	learner.SetOnLeaseTimeout(&onLeaseTimeout);
	
	acquireLease = false;
}

void PaxosLease::InitTransport()
{
	unsigned	i;
	Endpoint	endpoint;
	
	reader = new TransportUDPReader;
	if (!reader->Init(ReplicatedConfig::Get()->GetPort() + PLEASE_PORT_OFFSET))
		STOP_FAIL("cannot bind PaxosLease port", 1);
	reader->SetOnRead(&onRead);
	
	writers = (TransportWriter**) Alloc(sizeof(TransportWriter*) * ReplicatedConfig::Get()->numNodes);
	for (i = 0; i < ReplicatedConfig::Get()->numNodes; i++)
	{
		endpoint = ReplicatedConfig::Get()->endpoints[i];
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
	return learner.LeaseKnown();
}

unsigned PaxosLease::GetLeaseOwner()
{
	return learner.LeaseOwner();
}

uint64_t PaxosLease::GetLeaseEpoch()
{
	return learner.LeaseEpoch();
}

void PaxosLease::SetOnLearnLease(Callable* onLearnLeaseCallback_)
{
	onLearnLeaseCallback = onLearnLeaseCallback_;
	learner.SetOnLearnLease(onLearnLeaseCallback);
}

void PaxosLease::SetOnLeaseTimeout(Callable* onLeaseTimeoutCallback_)
{
	onLeaseTimeoutCallback = onLeaseTimeoutCallback_;
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

void PaxosLease::OnNewPaxosRound()
{
	proposer.OnNewPaxosRound();
}

void PaxosLease::OnLearnLease()
{
	Log_Trace();
	
	proposer.StopAcquireLease();
	Call(onLearnLeaseCallback);
}

void PaxosLease::OnLeaseTimeout()
{
	Log_Trace();
	
	if (acquireLease)
		proposer.StartAcquireLease();
	Call(onLeaseTimeoutCallback);
}
