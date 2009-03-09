#include "PLeaseProposer.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "Framework/Database/Transaction.h"
#include "PLeaseConsts.h"

PLeaseProposer::PLeaseProposer() :
	onRead(this, &PLeaseProposer::OnRead),
	onWrite(this, &PLeaseProposer::OnWrite),
	onAcquireLeaseTimeout(this, &PLeaseProposer::OnAcquireLeaseTimeout),
	acquireLeaseTimeout(MAX_LEASE_TIME, &onAcquireLeaseTimeout),
	onExtendLeaseTimeout(this, &PLeaseProposer::OnExtendLeaseTimeout),
	extendLeaseTimeout(&onExtendLeaseTimeout)
{
}

bool PLeaseProposer::Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_)
{
	// I/O framework
	ioproc = ioproc_;
	scheduler = scheduler_;
	config = config_;
	
	// Paxos variables
	state.Init();

	// TODO: read restart counter
	//if (!ReadState())
	{
		Log_Message("*** Paxos is starting from scratch (ReadState() failed) *** ");
	}
	
	if (!socket.Create(UDP)) return false;
	if (!socket.Bind(config->port + PLEASE_PROPOSER_PORT_OFFSET)) return false;
	if (!socket.SetNonblocking()) return false;
	
	udpread.fd = socket.fd;
	udpread.data = rdata;
	udpread.onComplete = &onRead;
	
	udpwrite.fd = socket.fd;
	udpwrite.data = wdata;
	udpwrite.onComplete = &onWrite;
	
	return ioproc->Add(&udpread);
}

void PLeaseProposer::AcquireLease()
{
	scheduler->Remove(&extendLeaseTimeout);
	
	if (!(state.preparing || state.proposing))
		StartPreparing();
}

void PLeaseProposer::OnRead()
{
	Log_Trace();
		
	Log_Message("Participant %d received message %.*s from %s",
		config->nodeID, udpread.data.length, udpread.data.buffer, udpread.endpoint.ToString());
	
	if (!msg.Read(udpread.data))
	{
		Log_Message("PLeaseMsg::Read() failed: invalid message format");
		ioproc->Add(&udpread); // read again
	}
	else
		ProcessMsg();
}

void PLeaseProposer::OnWrite()
{
	Log_Trace();

	numSent++;

	sending_to = config->endpoints.Next(sending_to);
	SendMessage();
}

void PLeaseProposer::BroadcastMessage()
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

void PLeaseProposer::SendMessage()
{
	if (sending_to != NULL)
	{
		udpwrite.endpoint = *sending_to;
		
		if (msg.type == PREPARE_REQUEST || msg.type == PROPOSE_REQUEST)
			udpwrite.endpoint.SetPort(udpwrite.endpoint.GetPort() + PLEASE_ACCEPTOR_PORT_OFFSET);
		else if (msg.type == LEARN_CHOSEN)
			udpwrite.endpoint.SetPort(udpwrite.endpoint.GetPort() + PLEASE_LEARNER_PORT_OFFSET);
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

void PLeaseProposer::ProcessMsg()
{
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
		if (msg.expireTime > Now())
		{
			state.highestReceivedProposalID = msg.acceptedProposalID;
			state.leaseOwner = msg.leaseOwner;
		}
	}
	
	bool read = TryFinishPreparing();
		
	if (read)
		ioproc->Add(&udpread);
}

// returns whether to put out an IORead
bool PLeaseProposer::TryFinishPreparing()
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
	
	return true;
}

void PLeaseProposer::OnProposeResponse()
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
bool PLeaseProposer::TryFinishProposing()
{
	Log_Trace();
	
	// see if we have enough positive replies to advance
	if (numAccepted >= config->MinMajority())
	{
		// a majority have accepted our proposal, we have consensus
		state.proposing = false;
		msg.LearnChosen(state.leaseOwner, state.expireTime);
		
		scheduler->Remove(&acquireLeaseTimeout);
		
		extendLeaseTimeout.Set(Now() + (state.expireTime - Now()) / 2);
		scheduler->Reset(&extendLeaseTimeout);
		
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
		
	msg.PrepareRequest(state.proposalID);
	
	BroadcastMessage();
}

void PLeaseProposer::StartProposing()
{
	Log_Trace();
	
	state.preparing = false;

	if (state.leaseOwner != config->nodeID)
	{
		// no point in getting someone else a lease
		// wait for OnAcquireLeaseTimeout
		return;
	}

	state.proposing = true;

	// acceptors get (t_e + S)
	msg.ProposeRequest(state.proposalID, state.leaseOwner, state.expireTime + MAX_CLOCK_SKEW);

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
