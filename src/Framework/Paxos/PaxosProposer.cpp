#include "PaxosProposer.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "Framework/Database/Transaction.h"
#include "PaxosConsts.h"

PaxosProposer::PaxosProposer() :
	onPrepareTimeout(this, &PaxosProposer::OnPrepareTimeout),
	onProposeTimeout(this, &PaxosProposer::OnProposeTimeout),
	prepareTimeout(PAXOS_TIMEOUT, &onPrepareTimeout),
	proposeTimeout(PAXOS_TIMEOUT, &onProposeTimeout)
{
}

void PaxosProposer::Init(TransportWriter** writers_, Scheduler* scheduler_, PaxosConfig* config_)
{
	writers = writers_;
	scheduler = scheduler_;
	config = config_;
	
	// Paxos variables
	paxosID = 0;
	state.Init();
}

void PaxosProposer::SetPaxosID(ulong64 paxosID_)
{
	paxosID = paxosID_;
}

bool PaxosProposer::Propose(ByteString& value)
{
	if (IsActive())
		return false;
	
	if (!state.value.Set(value))
		return false;

	if (state.leader && state.numProposals == 0)
	{
		state.numProposals++;
		StartProposing();
	}
	else
		StartPreparing();
	
	return true;
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
	
	for (unsigned nodeID = 0; nodeID < config->numNodes; nodeID++)
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
		numRejected++;
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
	
	if (numRejected >= ceil(config->numNodes / 2))
		StartPreparing();
	else if ((numReceived - numRejected) >= config->MinMajority())
		StartProposing();
	else if (numReceived == config->numNodes)
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
	if (numAccepted >= config->MinMajority())
	{
		// a majority have accepted our proposal, we have consensus
		StopProposing();
		msg.LearnChosen(paxosID, config->nodeID, LEARN_PROPOSAL, state.proposalID);
		BroadcastMessage();
	}
	else if (numReceived == config->numNodes)
		StartPreparing();
}

void PaxosProposer::StopPreparing()
{
	Log_Trace();

	state.preparing = false;
	scheduler->Remove(&prepareTimeout);
}

void PaxosProposer::StopProposing()
{
	Log_Trace();
	
	state.proposing = false;
	scheduler->Remove(&proposeTimeout);
}

void PaxosProposer::StartPreparing()
{
	Log_Trace();

	StopProposing();
	state.preparing = true;
	
	state.numProposals++;
	
	state.proposalID = config->NextHighest(state.proposalID);
	
	state.highestReceivedProposalID = 0; // TODO: should be -1 ?
	
	msg.PrepareRequest(paxosID, config->nodeID, state.proposalID);
	
	BroadcastMessage();
	
	scheduler->Reset(&prepareTimeout);
}

void PaxosProposer::StartProposing()
{
	Log_Trace();
	
	StopPreparing();

	state.proposing = true;

	msg.ProposeRequest(paxosID, config->nodeID, state.proposalID, state.value);

	BroadcastMessage();
	
	scheduler->Reset(&proposeTimeout);
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
