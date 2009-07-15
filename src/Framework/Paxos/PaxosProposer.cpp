#include "PaxosProposer.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"

PaxosProposer::PaxosProposer() :
	onPrepareTimeout(this, &PaxosProposer::OnPrepareTimeout),
	onProposeTimeout(this, &PaxosProposer::OnProposeTimeout),
	prepareTimeout(PAXOS_TIMEOUT, &onPrepareTimeout),
	proposeTimeout(PAXOS_TIMEOUT, &onProposeTimeout)
{
}

void PaxosProposer::Init(Writers writers_)
{
	writers = writers_;
	
	// Paxos variables
	paxosID = 0;
	state.Init();
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
	
	for (unsigned nodeID = 0; nodeID < RCONF->GetNumNodes(); nodeID++)
		writers[nodeID]->Write(wdata);
}

void PaxosProposer::OnPrepareResponse(PaxosMsg& msg_)
{
	Log_Trace();

	msg = msg_;

	if (!state.preparing || msg.proposalID != state.proposalID)
		return;
		
	numReceived++;
	
	if (msg.type == PAXOS_PREPARE_REJECTED)
	{
		if (msg.promisedProposalID > state.highestPromisedProposalID)
			state.highestPromisedProposalID = msg.promisedProposalID;
		numRejected++;
	}
	else if (msg.type == PAXOS_PREPARE_PREVIOUSLY_ACCEPTED &&
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
	
	if (numRejected >= ceil((double)(RCONF->GetNumNodes()) / 2))
		StartPreparing();
	else if ((numReceived - numRejected) >= RCONF->MinMajority())
		StartProposing();
	else if (numReceived == RCONF->GetNumNodes())
		StartPreparing();
}


void PaxosProposer::OnProposeResponse(PaxosMsg& msg_)
{
	Log_Trace();
	
	msg = msg_;
	
	if (!state.proposing || msg.proposalID != state.proposalID)
		return;
	
	numReceived++;
	
	if (msg.type == PAXOS_PROPOSE_ACCEPTED)
		numAccepted++;

	// see if we have enough positive replies to advance
	if (numAccepted >= RCONF->MinMajority())
	{
		// a majority have accepted our proposal, we have consensus
		StopProposing();
		msg.LearnProposal(paxosID, RCONF->GetNodeID(),
		state.proposalID);
		BroadcastMessage();
	}
	else if (numReceived == RCONF->GetNumNodes())
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
	
	state.proposalID = RCONF->NextHighest(
		max(state.proposalID, state.highestPromisedProposalID));
	
	state.highestReceivedProposalID = 0;
	
	msg.PrepareRequest(paxosID, RCONF->GetNodeID(),
	state.proposalID);
	
	BroadcastMessage();
	
	EventLoop::Reset(&prepareTimeout);
}

void PaxosProposer::StartProposing()
{
	Log_Trace();
	
	StopPreparing();

	state.proposing = true;

	msg.ProposeRequest(paxosID, RCONF->GetNodeID(),
	state.proposalID, state.value);

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
