#include "PaxosProposer.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "System/Events/EventLoop.h"
#include "Framework/Database/Transaction.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
#include "PaxosConsts.h"

PaxosProposer::PaxosProposer() :
	onPrepareTimeout(this, &PaxosProposer::OnPrepareTimeout),
	onProposeTimeout(this, &PaxosProposer::OnProposeTimeout),
	prepareTimeout(PAXOS_TIMEOUT, &onPrepareTimeout),
	proposeTimeout(PAXOS_TIMEOUT, &onProposeTimeout)
{
}

void PaxosProposer::Init(TransportWriter** writers_)
{
	writers = writers_;
	
	// Paxos variables
	paxosID = 0;
	state.Init();
}

void PaxosProposer::SetPaxosID(uint64_t paxosID_)
{
	paxosID = paxosID_;
}

bool PaxosProposer::Propose(ByteString& value)
{
	Log_Trace();
	
	if (IsActive())
	{
		ASSERT_FAIL();
		return false;
	}
	
	if (!state.value.Set(value))
	{
		ASSERT_FAIL();
		return false;
	}
	
	if (state.leader && state.numProposals == 0)
	{
		state.numProposals++;
		StartProposing();
	}
	else
		StartPreparing();
	
	return true;
}

void PaxosProposer::Stop()
{
	state.value.Init();
	state.preparing = false;
	state.proposing = false;
	EventLoop::Remove(&prepareTimeout);
	EventLoop::Remove(&proposeTimeout);
}

bool PaxosProposer::IsActive()
{
	return (state.preparing || state.proposing);
}

void PaxosProposer::BroadcastMessage()
{
	Log_Trace();
	
	numReceived = 0;
	numAccepted = 0;
	numRejected = 0;
	
	msg.Write(wdata);
	
	for (unsigned nodeID = 0; nodeID < ReplicatedConfig::Get()->numNodes; nodeID++)
		writers[nodeID]->Write(wdata);
}

void PaxosProposer::OnPrepareResponse(PaxosMsg& msg_)
{
	Log_Trace();

	msg = msg_;

	if (!state.preparing || msg.proposalID != state.proposalID)
		return;
		
	numReceived++;
	
	if (msg.subtype == PREPARE_REJECTED)
	{
		if (msg.promisedProposalID > state.highestPromisedProposalID)
			state.highestPromisedProposalID = msg.promisedProposalID;
		numRejected++;
	}
	else if (msg.subtype == PREPARE_PREVIOUSLY_ACCEPTED &&
			 msg.acceptedProposalID >= state.highestReceivedProposalID)
	{
		/* the >= could be replaced by > which would result in less copys
		 * however this would result in complications in multi paxos
		 * in the multi paxos steady state this branch is inactive
		 * it only runs after leader failure
		 * so it's ok
		 */
		state.highestReceivedProposalID = msg.acceptedProposalID;
		if (!state.value.Set(msg.value))
			ASSERT_FAIL();
	}
	
	if (numRejected >= ceil((double)(ReplicatedConfig::Get()->numNodes) / 2))
		StartPreparing();
	else if ((numReceived - numRejected) >= ReplicatedConfig::Get()->MinMajority())
		StartProposing();
	else if (numReceived == ReplicatedConfig::Get()->numNodes)
		StartPreparing();
}


void PaxosProposer::OnProposeResponse(PaxosMsg& msg_)
{
	Log_Trace();
	
	msg = msg_;
	
	if (!state.proposing || msg.proposalID != state.proposalID)
		return;
	
	numReceived++;
	
	if (msg.subtype == PROPOSE_ACCEPTED)
		numAccepted++;

	// see if we have enough positive replies to advance
	if (numAccepted >= ReplicatedConfig::Get()->MinMajority())
	{
		// a majority have accepted our proposal, we have consensus
		StopProposing();
		msg.LearnChosen(paxosID, ReplicatedConfig::Get()->nodeID, LEARN_PROPOSAL, state.proposalID);
		BroadcastMessage();
	}
	else if (numReceived == ReplicatedConfig::Get()->numNodes)
		StartPreparing();
}

void PaxosProposer::StopPreparing()
{
	Log_Trace();

	state.preparing = false;
	EventLoop::Remove(&prepareTimeout);
}

void PaxosProposer::StopProposing()
{
	Log_Trace();
	
	state.proposing = false;
	EventLoop::Remove(&proposeTimeout);
}

void PaxosProposer::StartPreparing()
{
	Log_Trace();

	StopProposing();
	state.preparing = true;
	
	state.numProposals++;
	
	state.proposalID = ReplicatedConfig::Get()->NextHighest(
		max(state.proposalID, state.highestPromisedProposalID));
	
	state.highestReceivedProposalID = 0; // TODO: should be -1 ?
	
	msg.PrepareRequest(paxosID, ReplicatedConfig::Get()->nodeID, state.proposalID);
	
	BroadcastMessage();
	
	EventLoop::Reset(&prepareTimeout);
}

void PaxosProposer::StartProposing()
{
	Log_Trace();
	
	StopPreparing();

	state.proposing = true;

	msg.ProposeRequest(paxosID, ReplicatedConfig::Get()->nodeID, state.proposalID, state.value);

	BroadcastMessage();
	
	EventLoop::Reset(&proposeTimeout);
}

void PaxosProposer::OnPrepareTimeout()
{
	Log_Trace();
	
	assert(state.preparing);
	
	StartPreparing();
}

void PaxosProposer::OnProposeTimeout()
{
	Log_Trace();
	
	assert(state.proposing);

	StartPreparing();
}
