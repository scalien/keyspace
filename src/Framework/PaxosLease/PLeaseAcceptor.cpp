#include "PLeaseAcceptor.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "PLeaseConsts.h"

PLeaseAcceptor::PLeaseAcceptor() :
	onRead(this, &PLeaseAcceptor::OnRead),
	onWrite(this, &PLeaseAcceptor::OnWrite),
	onLeaseTimeout(this, &PLeaseAcceptor::OnLeaseTimeout),
	leaseTimeout(&onLeaseTimeout)
{
}

bool PLeaseAcceptor::Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_)
{
	usleep((MAX_LEASE_TIME + MAX_CLOCK_SKEW) * 1000);

	ioproc = ioproc_;
	scheduler = scheduler_;
	config = config_;
	
	state.Init();
	
	if (!socket.Create(UDP)) return false;
	if (!socket.Bind(config->port + PLEASE_ACCEPTOR_PORT_OFFSET)) return false;
	if (!socket.SetNonblocking()) return false;
	
	udpread.fd = socket.fd;
	udpread.data = rdata;
	udpread.onComplete = &onRead;
	
	udpwrite.fd = socket.fd;
	udpwrite.data = wdata;
	udpwrite.onComplete = &onWrite;
	
	return ioproc->Add(&udpread);
}

void PLeaseAcceptor::SendReply()
{
	Log_Trace();
	
	udpwrite.endpoint = udpread.endpoint;
	
	if (!msg.Write(udpwrite.data))
	{
		Log_Trace();
		ioproc->Add(&udpread);
		return;
	}
	
	if (udpwrite.data.length > 0)
	{
		Log_Message("Participant %d sending message %.*s to %s",
			config->nodeID, udpwrite.data.length, udpwrite.data.buffer,
			udpwrite.endpoint.ToString());
		
		ioproc->Add(&udpwrite);
	}
	else
		ioproc->Add(&udpread);
}

void PLeaseAcceptor::OnRead()
{
	Log_Trace();
		
	Log_Message("Participant %d received message %.*s from %s",
		config->nodeID, udpread.data.length, udpread.data.buffer, udpread.endpoint.ToString());
	
	if (!msg.Read(udpread.data))
	{
		Log_Message("PLeaseMsg::Read() failed: invalid message format");
		ioproc->Add(&udpread); // read again
	} else
		ProcessMsg();
	
	assert(Xor(udpread.active, udpwrite.active));
}

void PLeaseAcceptor::OnWrite()
{
	Log_Trace();

	ioproc->Add(&udpread);
	
	assert(Xor(udpread.active, udpwrite.active));
}

void PLeaseAcceptor::ProcessMsg()
{
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
	
	if (msg.proposalID < state.promisedProposalID)
		msg.PrepareResponse(msg.proposalID, PREPARE_REJECTED);
	else
	{
		state.promisedProposalID = msg.proposalID;

		if (!state.accepted)
			msg.PrepareResponse(msg.proposalID, PREPARE_CURRENTLY_OPEN);
		else
			msg.PrepareResponse(msg.proposalID, PREPARE_PREVIOUSLY_ACCEPTED,
				state.acceptedProposalID, state.acceptedLeaseOwner, state.acceptedExpireTime);
	}
	
	SendReply();
}

void PLeaseAcceptor::OnProposeRequest()
{
	Log_Trace();
	
	if (msg.expireTime > Now())
	{
		if (msg.proposalID < state.promisedProposalID)
			msg.ProposeResponse(msg.proposalID, PROPOSE_REJECTED);
		else
		{
			state.accepted = true;
			state.acceptedProposalID = msg.proposalID;
			state.acceptedLeaseOwner = msg.leaseOwner;
			state.acceptedExpireTime = msg.expireTime;
			
			leaseTimeout.Set(state.acceptedExpireTime);
			scheduler->Reset(&leaseTimeout);
			
			msg.ProposeResponse(msg.proposalID, PROPOSE_ACCEPTED);
		}
		
		SendReply();
	}
	else
	{
		Log_Message("Expired propose request received (msg.expireTime = %llu | Now = %llu)",
			msg.expireTime, Now());
		ioproc->Add(&udpread);
	}
}

void PLeaseAcceptor::OnLeaseTimeout()
{
	Log_Trace();
	
	state.OnLeaseTimeout();
}
