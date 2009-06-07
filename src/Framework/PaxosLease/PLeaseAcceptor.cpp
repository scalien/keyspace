#include "PLeaseAcceptor.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "System/Events/EventLoop.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
#include "PLeaseConsts.h"

PLeaseAcceptor::PLeaseAcceptor() :
	onLeaseTimeout(this, &PLeaseAcceptor::OnLeaseTimeout),
	leaseTimeout(&onLeaseTimeout)
{
}

void PLeaseAcceptor::Init(TransportWriter** writers_)
{
	Log_Message("*** Waiting %d milliseconds... (this is normal) ***",
	MAX_LEASE_TIME + MAX_CLOCK_SKEW);

	usleep((MAX_LEASE_TIME + MAX_CLOCK_SKEW) * 1000);
	
	writers = writers_;
	
	state.Init();
}

void PLeaseAcceptor::SendReply(unsigned nodeID)
{
	msg.Write(wdata);

	writers[nodeID]->Write(wdata);
}

void PLeaseAcceptor::ProcessMsg(PLeaseMsg& msg_)
{
	msg = msg_;
	
	if (msg.type == PLEASE_PREPARE_REQUEST)
		OnPrepareRequest();
	else if (msg.type == PLEASE_PROPOSE_REQUEST)
		OnProposeRequest();
	else
		ASSERT_FAIL();
}

void PLeaseAcceptor::OnPrepareRequest()
{
	Log_Trace();
	
	Log_Message("msg.paxosID: %" PRIu64 ", my.paxosID: %" PRIu64 "",
	msg.paxosID, ReplicatedLog::Get()->GetPaxosID());

	if (msg.paxosID < ReplicatedLog::Get()->GetPaxosID())
		return; // only up-to-date nodes can become masters

	ReplicatedLog::Get()->OnPaxosLeaseMsg(msg.paxosID, msg.nodeID);
	
	unsigned senderID = msg.nodeID;
	
	if (state.accepted && state.acceptedExpireTime < Now())
	{
		EventLoop::Remove(&leaseTimeout);
		OnLeaseTimeout();
	}
	
	if (msg.proposalID < state.promisedProposalID)
		msg.PrepareRejected(ReplicatedConfig::Get()->nodeID, msg.proposalID);
	else
	{
		state.promisedProposalID = msg.proposalID;

		if (!state.accepted)
			msg.PrepareCurrentlyOpen(ReplicatedConfig::Get()->nodeID, msg.proposalID);
		else
			msg.PreparePreviouslyAccepted(ReplicatedConfig::Get()->nodeID,
			msg.proposalID, state.acceptedProposalID,
			state.acceptedLeaseOwner, state.acceptedExpireTime);
	}
	
	SendReply(senderID);
}

void PLeaseAcceptor::OnProposeRequest()
{
	Log_Trace();
	
	unsigned senderID = msg.nodeID;
	
	if (state.accepted && state.acceptedExpireTime < Now())
	{
		EventLoop::Remove(&leaseTimeout);
		OnLeaseTimeout();
	}

	if (msg.expireTime < Now())
	{
		Log_Message("Expired propose request received (msg.expireTime = %" PRIu64 " | Now = %" PRIu64 ")",
			msg.expireTime, Now());
		return;
	}
	
	if (msg.expireTime > Now() + MAX_LEASE_TIME + 2 * MAX_CLOCK_SKEW)
		STOP_FAIL("Clock skew between nodes exceeds allowed maximum", 1);

	if (msg.proposalID < state.promisedProposalID)
		msg.ProposeRejected(ReplicatedConfig::Get()->nodeID, msg.proposalID);
	else
	{
		state.accepted = true;
		state.acceptedProposalID = msg.proposalID;
		state.acceptedLeaseOwner = msg.leaseOwner;
		state.acceptedExpireTime = msg.expireTime;
		
		leaseTimeout.Set(state.acceptedExpireTime);
		EventLoop::Reset(&leaseTimeout);
		
		msg.ProposeAccepted(ReplicatedConfig::Get()->nodeID, msg.proposalID);
	}
	
	SendReply(senderID);
}

void PLeaseAcceptor::OnLeaseTimeout()
{
	Log_Trace();
	
	state.OnLeaseTimeout();
}
