#include "PLeaseAcceptor.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "System/Events/EventLoop.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Framework/Paxos/PaxosConfig.h"
#include "PLeaseConsts.h"

PLeaseAcceptor::PLeaseAcceptor() :
	onLeaseTimeout(this, &PLeaseAcceptor::OnLeaseTimeout),
	leaseTimeout(&onLeaseTimeout)
{
}

void PLeaseAcceptor::Init(TransportWriter** writers_)
{
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
	
	if (msg.type == PREPARE_REQUEST)
		OnPrepareRequest();
	else if (msg.type == PROPOSE_REQUEST)
		OnProposeRequest();
	else
		ASSERT_FAIL();
}

void PLeaseAcceptor::OnPrepareRequest()
{
	Log_Trace();
	
	Log_Message("msg.paxosID: %" PRIu64 ", my.paxosID: %" PRIu64 "", msg.paxosID,
		ReplicatedLog::Get()->GetPaxosID());
	
	if (msg.paxosID < ReplicatedLog::Get()->GetPaxosID())
		return; // only up-to-date nodes can become masters
	
	unsigned senderID = msg.nodeID;
	
	if (state.accepted && state.acceptedExpireTime < Now())
	{
		EventLoop::Remove(&leaseTimeout);
		OnLeaseTimeout();
	}
	
	if (msg.proposalID < state.promisedProposalID)
		msg.PrepareResponse(PaxosConfig::Get()->nodeID, msg.proposalID, PREPARE_REJECTED);
	else
	{
		state.promisedProposalID = msg.proposalID;

		if (!state.accepted)
			msg.PrepareResponse(PaxosConfig::Get()->nodeID, msg.proposalID, PREPARE_CURRENTLY_OPEN);
		else
			msg.PrepareResponse(PaxosConfig::Get()->nodeID, msg.proposalID, PREPARE_PREVIOUSLY_ACCEPTED,
				state.acceptedProposalID, state.acceptedLeaseOwner, state.acceptedExpireTime);
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
	
	if (msg.expireTime > Now() + MAX_LEASE_TIME + MAX_CLOCK_SKEW)
		STOP_FAIL("Clock skew between nodes exceeds allowed maximum", 1);

	if (msg.proposalID < state.promisedProposalID)
		msg.ProposeResponse(PaxosConfig::Get()->nodeID, msg.proposalID, PROPOSE_REJECTED);
	else
	{
		state.accepted = true;
		state.acceptedProposalID = msg.proposalID;
		state.acceptedLeaseOwner = msg.leaseOwner;
		state.acceptedExpireTime = msg.expireTime;
		
		leaseTimeout.Set(state.acceptedExpireTime);
		EventLoop::Reset(&leaseTimeout);
		
		msg.ProposeResponse(PaxosConfig::Get()->nodeID, msg.proposalID, PROPOSE_ACCEPTED);
	}
	
	SendReply(senderID);
}

void PLeaseAcceptor::OnLeaseTimeout()
{
	Log_Trace();
	
	state.OnLeaseTimeout();
}
