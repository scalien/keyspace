#include "PLeaseProposer.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "Framework/Database/Transaction.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "PLeaseConsts.h"

PLeaseProposer::PLeaseProposer() :
	onAcquireLeaseTimeout(this, &PLeaseProposer::OnAcquireLeaseTimeout),
	acquireLeaseTimeout(MAX_LEASE_TIME, &onAcquireLeaseTimeout),
	onExtendLeaseTimeout(this, &PLeaseProposer::OnExtendLeaseTimeout),
	extendLeaseTimeout(&onExtendLeaseTimeout)
{
}

void PLeaseProposer::Init(ReplicatedLog* replicatedLog_, TransportWriter** writers_,
						  Scheduler* scheduler_, PaxosConfig* config_)
{
	replicatedLog = replicatedLog_;
	writers = writers_;
	scheduler = scheduler_;
	config = config_;
	
	state.Init();

	// TODO: read restart counter
	//if (!ReadState())
	{
		Log_Message("*** Paxos is starting from scratch (ReadState() failed) *** ");
	}
}

void PLeaseProposer::AcquireLease()
{
	scheduler->Remove(&extendLeaseTimeout);
	
	if (!(state.preparing || state.proposing))
		StartPreparing();
}

void PLeaseProposer::BroadcastMessage()
{
	Log_Trace();
	
	numReceived = 0;
	numAccepted = 0;
	numRejected = 0;
	
	msg.Write(wdata);
	
	for (unsigned nodeID = 0; nodeID < config->numNodes; nodeID++)
		writers[nodeID]->Write(wdata);
}

void PLeaseProposer::ProcessMsg(PLeaseMsg &msg_)
{
	msg = msg_;

	if (msg.type == PREPARE_RESPONSE)
		OnPrepareResponse();
	else if (msg.type == PROPOSE_RESPONSE)
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
	
	if (msg.response == PREPARE_REJECTED)
		numRejected++;
	else if (msg.response == PREPARE_PREVIOUSLY_ACCEPTED && 
			 msg.acceptedProposalID >= state.highestReceivedProposalID &&
			 msg.expireTime > Now())
	{
		state.highestReceivedProposalID = msg.acceptedProposalID;
		state.leaseOwner = msg.leaseOwner;
	}

	if (numRejected >= ceil(config->numNodes / 2))
	{
		StartPreparing();
		return;
	}
	
	// see if we have enough positive replies to advance	
	if ((numReceived - numRejected) >= config->MinMajority())
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
	
	if (msg.response == PROPOSE_ACCEPTED)
		numAccepted++;

	// see if we have enough positive replies to advance
	if (numAccepted >= config->MinMajority())
	{
		// a majority have accepted our proposal, we have consensus
		state.proposing = false;
		msg.LearnChosen(config->nodeID, state.leaseOwner, state.expireTime);
		
		scheduler->Remove(&acquireLeaseTimeout);
		
		extendLeaseTimeout.Set(Now() + (state.expireTime - Now()) / 2);
		scheduler->Reset(&extendLeaseTimeout);
		
		BroadcastMessage();
		return;
	}
	
	if (numReceived == config->numNodes)
		StartPreparing();
}

void PLeaseProposer::StartPreparing()
{
	Log_Trace();

	scheduler->Reset(&acquireLeaseTimeout);

	state.proposing = false;

	state.preparing = true;
	
	state.leaseOwner = config->nodeID;
	state.expireTime = Now() + MAX_LEASE_TIME;
	
	state.highestReceivedProposalID = 0;

	state.proposalID = config->NextHighest(state.proposalID);
		
	msg.PrepareRequest(config->nodeID, state.proposalID, replicatedLog->GetPaxosID());
	
	BroadcastMessage();
}

void PLeaseProposer::StartProposing()
{
	Log_Trace();
	
	state.preparing = false;

	if (state.leaseOwner != config->nodeID)
		return; // no point in getting someone else a lease, wait for OnAcquireLeaseTimeout

	state.proposing = true;

	// acceptors get (t_e + S)
	msg.ProposeRequest(config->nodeID, state.proposalID,
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
