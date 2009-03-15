#include "PLeaseAcceptor.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "PLeaseConsts.h"

PLeaseAcceptor::PLeaseAcceptor() :
	onLeaseTimeout(this, &PLeaseAcceptor::OnLeaseTimeout),
	leaseTimeout(&onLeaseTimeout)
{
}

void PLeaseAcceptor::Init(TransportWriter** writers_, Scheduler* scheduler_, PaxosConfig* config_)
{
	usleep((MAX_LEASE_TIME + MAX_CLOCK_SKEW) * 1000);

	writers = writers_;
	scheduler = scheduler_;
	config = config_;
	
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
	
	unsigned senderID = msg.nodeID;
	
	if (state.accepted && state.acceptedExpireTime < Now())
	{
		scheduler->Remove(&leaseTimeout);
		OnLeaseTimeout();
	}
	
	if (msg.proposalID < state.promisedProposalID)
		msg.PrepareResponse(config->nodeID, msg.proposalID, PREPARE_REJECTED);
	else
	{
		state.promisedProposalID = msg.proposalID;

		if (!state.accepted)
			msg.PrepareResponse(config->nodeID, msg.proposalID, PREPARE_CURRENTLY_OPEN);
		else
			msg.PrepareResponse(config->nodeID, msg.proposalID, PREPARE_PREVIOUSLY_ACCEPTED,
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
		scheduler->Remove(&leaseTimeout);
		OnLeaseTimeout();
	}

	if (msg.expireTime < Now())
	{
		Log_Message("Expired propose request received (msg.expireTime = %llu | Now = %llu)",
			msg.expireTime, Now());
		return;
	}

	if (msg.proposalID < state.promisedProposalID)
		msg.ProposeResponse(config->nodeID, msg.proposalID, PROPOSE_REJECTED);
	else
	{
		state.accepted = true;
		state.acceptedProposalID = msg.proposalID;
		state.acceptedLeaseOwner = msg.leaseOwner;
		state.acceptedExpireTime = msg.expireTime;
		
		leaseTimeout.Set(state.acceptedExpireTime);
		scheduler->Reset(&leaseTimeout);
		
		msg.ProposeResponse(config->nodeID, msg.proposalID, PROPOSE_ACCEPTED);
	}
	
	SendReply(senderID);
}

void PLeaseAcceptor::OnLeaseTimeout()
{
	Log_Trace();
	
	state.OnLeaseTimeout();
}
