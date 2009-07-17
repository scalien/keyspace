#include "PLeaseProposer.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "System/Events/EventLoop.h"
#include "Framework/Database/Transaction.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
#include "PLeaseConsts.h"

PLeaseProposer::PLeaseProposer() :
	onAcquireLeaseTimeout(this, &PLeaseProposer::OnAcquireLeaseTimeout),
	acquireLeaseTimeout(ACQUIRELEASE_TIMEOUT, &onAcquireLeaseTimeout),
	onExtendLeaseTimeout(this, &PLeaseProposer::OnExtendLeaseTimeout),
	extendLeaseTimeout(&onExtendLeaseTimeout)
{
}

void PLeaseProposer::Init(Writers writers_)
{
	writers = writers_;
	
	highestProposalID = 0;
	
	state.Init();
}

void PLeaseProposer::StartAcquireLease()
{
	Log_Trace();
	
	EventLoop::Remove(&extendLeaseTimeout);
	
	if (!(state.preparing || state.proposing))
		StartPreparing();
}

void PLeaseProposer::StopAcquireLease()
{
	Log_Trace();
	
	state.preparing = false;
	state.proposing = false;
	EventLoop::Remove(&extendLeaseTimeout);
	EventLoop::Remove(&acquireLeaseTimeout);
}

void PLeaseProposer::OnNewPaxosRound()
{
	// since PaxosLease msgs carry the paxosID, and nodes
	// won't reply if their paxosID is larges than the msg's
	// if the paxosID increases we must restart the
	// PaxosLease round, if it's active
	// only restart if we're masters
	
	Log_Trace();
	
	if (acquireLeaseTimeout.IsActive() && RLOG->IsMaster())
		StartPreparing();
}

void PLeaseProposer::BroadcastMessage()
{
	Log_Trace();
	
	unsigned nodeID;
	
	numReceived = 0;
	numAccepted = 0;
	numRejected = 0;
	
	msg.Write(wdata);
	
	for (nodeID = 0; nodeID < RCONF->GetNumNodes(); nodeID++)
		writers[nodeID]->Write(wdata);
}

void PLeaseProposer::ProcessMsg(PLeaseMsg &msg_)
{
	msg = msg_;

	if (msg.IsPrepareResponse())
		OnPrepareResponse();
	else if (msg.IsProposeResponse())
		OnProposeResponse();
	else
		ASSERT_FAIL();
}

void PLeaseProposer::OnPrepareResponse()
{
	Log_Trace();

	if (state.expireTime < Now())
		return; // wait for timer

	if (!state.preparing || msg.proposalID != state.proposalID)
		return;
		
	numReceived++;
	
	if (msg.type == PLEASE_PREPARE_REJECTED)
		numRejected++;
	else if (msg.type == PLEASE_PREPARE_PREVIOUSLY_ACCEPTED && 
			 msg.acceptedProposalID >= state.highestReceivedProposalID &&
			 msg.expireTime > Now())
	{
		state.highestReceivedProposalID = msg.acceptedProposalID;
		state.leaseOwner = msg.leaseOwner;
	}

	if (numRejected >= ceil((double)(RCONF->GetNumNodes()) / 2))
	{
		StartPreparing();
		return;
	}
	
	// see if we have enough positive replies to advance	
	if ((numReceived - numRejected) >= RCONF->MinMajority())
		StartProposing();	
}

void PLeaseProposer::OnProposeResponse()
{
	Log_Trace();

	if (state.expireTime < Now())
		return; // wait for timer
	
	if (!state.proposing || msg.proposalID != state.proposalID)
		return;
	
	numReceived++;
	
	if (msg.type == PLEASE_PROPOSE_ACCEPTED)
		numAccepted++;

	// see if we have enough positive replies to advance
	if (numAccepted >= RCONF->MinMajority())
	{
		// a majority have accepted our proposal, we have consensus
		state.proposing = false;
		msg.LearnChosen(RCONF->GetNodeID(),
		state.leaseOwner, state.expireTime);
		
		EventLoop::Remove(&acquireLeaseTimeout);
		
		extendLeaseTimeout.Set(Now() + (state.expireTime - Now()) / 5);
		EventLoop::Reset(&extendLeaseTimeout);
	
		BroadcastMessage();
		return;
	}
	
	if (numReceived == RCONF->GetNumNodes())
		StartPreparing();
}

void PLeaseProposer::StartPreparing()
{
	Log_Trace();

	EventLoop::Reset(&acquireLeaseTimeout);

	state.proposing = false;

	state.preparing = true;
	
	state.leaseOwner = RCONF->GetNodeID();
	state.expireTime = Now() + MAX_LEASE_TIME;
	
	state.highestReceivedProposalID = 0;

	state.proposalID = RCONF->NextHighest(highestProposalID);
		
	if (state.proposalID > highestProposalID)
		highestProposalID = state.proposalID;
	
	msg.PrepareRequest(RCONF->GetNodeID(),
	state.proposalID, RLOG->GetPaxosID());
	
	BroadcastMessage();
}

void PLeaseProposer::StartProposing()
{
	Log_Trace();
	
	state.preparing = false;

	if (state.leaseOwner != RCONF->GetNodeID())
		return; // no point in getting someone else a lease,
				// wait for OnAcquireLeaseTimeout

	state.proposing = true;

	// acceptors get (t_e + S)
	msg.ProposeRequest(RCONF->GetNodeID(), state.proposalID,
		state.leaseOwner, state.expireTime + MAX_CLOCK_SKEW);

	BroadcastMessage();
}

void PLeaseProposer::OnAcquireLeaseTimeout()
{
	Log_Trace();
		
	StartPreparing();
}

void PLeaseProposer::OnExtendLeaseTimeout()
{
	Log_Trace();
	
	assert(!(state.preparing || state.proposing));
	
	StartPreparing();
}
