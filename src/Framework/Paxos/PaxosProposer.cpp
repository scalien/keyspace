#include "PaxosProposer.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "Framework/Database/Transaction.h"
#include "PaxosConsts.h"

PaxosProposer::PaxosProposer() :
	onRead(this, &PaxosProposer::OnRead),
	onWrite(this, &PaxosProposer::OnWrite),
	onPrepareTimeout(this, &PaxosProposer::OnPrepareTimeout),
	onProposeTimeout(this, &PaxosProposer::OnProposeTimeout),
	prepareTimeout(PAXOS_TIMEOUT, &onPrepareTimeout),
	proposeTimeout(PAXOS_TIMEOUT, &onProposeTimeout)
{
}

bool PaxosProposer::Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_)
{
	// I/O framework
	ioproc = ioproc_;
	scheduler = scheduler_;
	config = config_;
	
	// Paxos variables
	paxosID = 0;
	state.Init();
	
	if (!socket.Create(UDP)) return false;
	if (!socket.Bind(config->port + PAXOS_PROPOSER_PORT_OFFSET)) return false;
	if (!socket.SetNonblocking()) return false;
	
	udpread.fd = socket.fd;
	udpread.data = rdata;
	udpread.onComplete = &onRead;
	
	udpwrite.fd = socket.fd;
	udpwrite.data = wdata;
	udpwrite.onComplete = &onWrite;
	
	return ioproc->Add(&udpread);
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
	{
		StartPreparing();
	}
	
	return true;
}

bool PaxosProposer::IsActive()
{
	return (state.preparing || state.proposing);
}

void PaxosProposer::OnRead()
{
	Log_Trace();
		
	Log_Message("Participant %d received message %.*s from %s",
		config->nodeID, udpread.data.length, udpread.data.buffer, udpread.endpoint.ToString());
	
	if (!msg.Read(udpread.data))
	{
		Log_Message("PaxosMsg::Read() failed: invalid message format");
		ioproc->Add(&udpread); // read again
	}
	else
		ProcessMsg();
}

void PaxosProposer::OnWrite()
{
	Log_Trace();

	numSent++;

	sending_to = config->endpoints.Next(sending_to);

	SendMessage();
}

void PaxosProposer::BroadcastMessage()
{
	Log_Trace();

	// remove outstanding read (we're called from a timeout)
	if (udpread.active)
		ioproc->Remove(&udpread);

	numSent		= 0;
	numReceived = 0;
	numAccepted = 0;
	numRejected = 0;

	if (!msg.Write(udpwrite.data))
	{
		ioproc->Add(&udpread);
		return;
	}
	
	sending_to = config->endpoints.Head();
	SendMessage();
}

void PaxosProposer::SendMessage()
{
	if (sending_to != NULL)
	{
		udpwrite.endpoint = *sending_to;
		
		if (msg.type == PREPARE_REQUEST || msg.type == PROPOSE_REQUEST)
			udpwrite.endpoint.SetPort(udpwrite.endpoint.GetPort() + PAXOS_ACCEPTOR_PORT_OFFSET);
		else if (msg.type == LEARN_CHOSEN)
			udpwrite.endpoint.SetPort(udpwrite.endpoint.GetPort() + PAXOS_LEARNER_PORT_OFFSET);
		else
			ASSERT_FAIL();
		
		Log_Message("Participant %d sending message %.*s to %s",
			config->nodeID, udpwrite.data.length, udpwrite.data.buffer,
			udpwrite.endpoint.ToString());
		
		ioproc->Add(&udpwrite);
	}
	else
		ioproc->Add(&udpread);
}

void PaxosProposer::ProcessMsg()
{
	if (msg.type == PREPARE_RESPONSE)
		OnPrepareResponse();
	else if (msg.type == PROPOSE_RESPONSE)
		OnProposeResponse();
	else
		ASSERT_FAIL();
}

void PaxosProposer::OnPrepareResponse()
{
	Log_Trace();

	if (!state.preparing || msg.proposalID != state.proposalID)
	{
		ioproc->Add(&udpread);
		return;
	}
		
	numReceived++;
	
	if (msg.response == PREPARE_REJECTED)
		numRejected++;
	else if (msg.response == PREPARE_PREVIOUSLY_ACCEPTED &&
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
	
	bool read = TryFinishPreparing();
		
	if (read)
		ioproc->Add(&udpread);
}

// returns whether to put out an IORead
bool PaxosProposer::TryFinishPreparing()
{
	Log_Trace();
			
	if (numRejected >= ceil(config->numNodes / 2))
	{
		StartPreparing();
		return false;
	}
	
	// see if we have enough positive replies to advance	
	int numPositive = numReceived - numRejected;
	if (numPositive >= config->MinMajority())
	{
		StartProposing();
		return false;
	}
	
	if (numReceived == numSent)
	{
		StartPreparing();
		return false;
	}
	
	return true;
}

void PaxosProposer::OnProposeResponse()
{
	Log_Trace();
	
	if (!state.proposing || msg.proposalID != state.proposalID)
	{
		ioproc->Add(&udpread);
		return;
	}
	
	numReceived++;
	
	if (msg.response == PROPOSE_ACCEPTED)
		numAccepted++;

	bool read = TryFinishProposing();
		
	if (read)
		ioproc->Add(&udpread);
}

// returns whether to put out an IORead
bool PaxosProposer::TryFinishProposing()
{
	Log_Trace();
	
	// see if we have enough positive replies to advance
	if (numAccepted >= config->MinMajority())
	{
		// a majority have accepted our proposal, we have consensus
		StopProposing();
		msg.LearnChosen(paxosID, state.value);
		BroadcastMessage();
		return false;
	}
	
	if (numReceived == numSent)
	{
		StartPreparing();
		return false;
	}
	
	return true;
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
	
	msg.PrepareRequest(paxosID, state.proposalID);
	
	BroadcastMessage();
	
	scheduler->Reset(&prepareTimeout);
}

void PaxosProposer::StartProposing()
{
	Log_Trace();
	
	StopPreparing();

	state.proposing = true;

	msg.ProposeRequest(paxosID, state.proposalID, state.value);

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
