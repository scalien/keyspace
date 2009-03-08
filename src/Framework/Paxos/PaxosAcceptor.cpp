#include "PaxosAcceptor.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "Framework/Database/Transaction.h"
#include "PaxosConsts.h"

PaxosAcceptor::PaxosAcceptor() :
	onRead(this, &PaxosAcceptor::OnRead),
	onWrite(this, &PaxosAcceptor::OnWrite),
	onDBComplete(this, &PaxosAcceptor::OnDBComplete)
{
	mdbop.SetCallback(&onDBComplete);
}

bool PaxosAcceptor::Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_)
{
	// I/O framework
	ioproc = ioproc_;
	scheduler = scheduler_;
	config = config_;
	
	table = database.GetTable("state");
	if (table == NULL)
	{
		Log_Trace();
		return false;
	}
	transaction.Set(table);

	// Paxos variables
	paxosID = 0;
	state.Init();

	//if (!ReadState())
	{
		Log_Message("*** Paxos is starting from scratch (ReadState() failed) *** ");
	}
	
	if (!socket.Create(UDP)) return false;
	if (!socket.Bind(config->port + PAXOS_ACCEPTOR_PORT_OFFSET)) return false;
	if (!socket.SetNonblocking()) return false;
	
	udpread.fd = socket.fd;
	udpread.data = rdata;
	udpread.onComplete = &onRead;
	
	udpwrite.fd = socket.fd;
	udpwrite.data = wdata;
	udpwrite.onComplete = &onWrite;
	
	return ioproc->Add(&udpread);
}

void PaxosAcceptor::SetPaxosID(ulong64 paxosID_)
{
	paxosID = paxosID_;
}

bool PaxosAcceptor::ReadState()
{
	Log_Trace();
	
	bool ret;
	int nread;
	
	if (table == NULL)
		return false;

	ret = true;
	ret = ret && table->Get(NULL, "paxosID",			bytearrays[0]);
	ret = ret && table->Get(NULL, "accepted",			bytearrays[1]);
	ret = ret && table->Get(NULL, "promisedProposalID",	bytearrays[2]);
	ret = ret && table->Get(NULL, "acceptedProposalID",	bytearrays[3]);
	ret = ret && table->Get(NULL, "acceptedValue",		state.acceptedValue);

	if (!ret)
		return false;

	paxosID = strntoulong64(bytearrays[0].buffer, bytearrays[0].length, &nread);
	if (nread != bytearrays[0].length)
	{
		Log_Trace();
		return false;
	}
	
	state.accepted = strntol(bytearrays[1].buffer, bytearrays[1].length, &nread);
	if (nread != bytearrays[1].length)
	{
		Log_Trace();
		return false;
	}
	
	state.promisedProposalID = strntoulong64(bytearrays[2].buffer, bytearrays[2].length, &nread);
	if (nread != bytearrays[2].length)
	{
		Log_Trace();
		return false;
	}
	
	state.acceptedProposalID = strntoulong64(bytearrays[3].buffer, bytearrays[3].length, &nread);
	if (nread != bytearrays[3].length)
	{
		Log_Trace();
		return false;
	}
	
	return true;
}

bool PaxosAcceptor::WriteState(Transaction* transaction)
{
	Log_Trace();

	bool ret;
	
	if (table == NULL)
		return false;
	
	bytearrays[0].Set(rprintf("%llu",			paxosID));
	bytearrays[1].Set(rprintf("%d",				state.accepted));
	bytearrays[2].Set(rprintf("%llu",			state.promisedProposalID));
	bytearrays[3].Set(rprintf("%llu",			state.acceptedProposalID));
	
	mdbop.Init();
	
	ret = true;
	ret = ret && mdbop.Set(table, "paxosID",			bytearrays[0]);
	ret = ret && mdbop.Set(table, "accepted",			bytearrays[1]);
	ret = ret && mdbop.Set(table, "promisedProposalID",	bytearrays[2]);
	ret = ret && mdbop.Set(table, "acceptedProposalID",	bytearrays[3]);
	ret = ret && mdbop.Set(table, "acceptedValue",		state.acceptedValue);

	if (!ret)
		return false;
	
	mdbop.SetTransaction(transaction);
	
	dbWriter.Add(&mdbop);

	return true;
}


void PaxosAcceptor::SendReply()
{
	Log_Trace();
		
	if (!msg.Write(udpwrite.data))
	{
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

void PaxosAcceptor::OnRead()
{
	Log_Trace();
		
	Log_Message("Participant %d received message %.*s from %s",
		config->nodeID, udpread.data.length, udpread.data.buffer, udpread.endpoint.ToString());
	
	if (!msg.Read(udpread.data))
	{
		Log_Message("PaxosMsg::Read() failed: invalid message format");
		ioproc->Add(&udpread); // read again
	} else
		ProcessMsg();
	
	assert(Xor(udpread.active, udpwrite.active, mdbop.active));
}

void PaxosAcceptor::OnWrite()
{
	Log_Trace();

	ioproc->Add(&udpread);
	
	assert(Xor(udpread.active, udpwrite.active, mdbop.active));
}

void PaxosAcceptor::ProcessMsg()
{
	if (msg.type == PREPARE_REQUEST)
		OnPrepareRequest();
	else if (msg.type == PROPOSE_REQUEST)
		OnProposeRequest();
	else
		ASSERT_FAIL();
}

void PaxosAcceptor::OnPrepareRequest()
{
	Log_Trace();
	
	if (msg.proposalID < state.promisedProposalID)
	{
		msg.PrepareResponse(msg.paxosID, msg.proposalID, PREPARE_REJECTED);
		
		udpwrite.endpoint = udpread.endpoint;
		if (!msg.Write(udpwrite.data))
		{
			Log_Trace();
			ioproc->Add(&udpread);
			return;
		}
		
		SendReply();
		return;
	}

	state.promisedProposalID = msg.proposalID;

	if (!state.accepted)
		msg.PrepareResponse(msg.paxosID, msg.proposalID, PREPARE_CURRENTLY_OPEN);
	else
		msg.PrepareResponse(msg.paxosID, msg.proposalID, PREPARE_PREVIOUSLY_ACCEPTED,
			state.acceptedProposalID, state.acceptedValue);
	
	WriteState(&transaction);
}

void PaxosAcceptor::OnProposeRequest()
{
	Log_Trace();
	
	if (msg.proposalID < state.promisedProposalID)
	{
		msg.ProposeResponse(msg.paxosID, msg.proposalID, PROPOSE_REJECTED);
		
		udpwrite.endpoint = udpread.endpoint;
		if (!msg.Write(udpwrite.data))
		{
			Log_Trace();
			ioproc->Add(&udpread);
			return;
		}
		
		SendReply();
		return;
	}

	state.accepted = true;
	state.acceptedProposalID = msg.proposalID;
	if (!state.acceptedValue.Set(msg.value))
		ASSERT_FAIL();
	msg.ProposeResponse(msg.paxosID, msg.proposalID, PROPOSE_ACCEPTED);
	
	WriteState(&transaction);	
}

void PaxosAcceptor::OnDBComplete()
{
	Log_Trace();

	// TODO: check that the transaction commited

	udpwrite.endpoint = udpread.endpoint;
	if (!msg.Write(udpwrite.data))
	{
		Log_Trace();
		ioproc->Add(&udpread);
		return;
	}
	
	SendReply();
	
	assert(Xor(udpread.active, udpwrite.active, mdbop.active));
}
