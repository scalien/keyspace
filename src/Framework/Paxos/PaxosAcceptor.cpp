#include "PaxosAcceptor.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "System/Events/EventLoop.h"
#include "Framework/Database/Transaction.h"
#include "PaxosConfig.h"
#include "PaxosConsts.h"

PaxosAcceptor::PaxosAcceptor() :
	onDBComplete(this, &PaxosAcceptor::OnDBComplete)
{
	mdbop.SetCallback(&onDBComplete);
}

void PaxosAcceptor::Init(TransportWriter** writers_)
{
	writers = writers_;
	
	table = database.GetTable("keyspace");
	if (table == NULL)
		ASSERT_FAIL();
	transaction.Set(table);

	// Paxos variables
	paxosID = 0;
	state.Init();

	if (!ReadState())
		Log_Message("*** Paxos is starting from scratch (ReadState() failed) *** ");
}

bool PaxosAcceptor::Persist(Transaction* transaction)
{
	Log_Trace();

	bool ret;
	
	if (table == NULL)
		return false;
		
	bytearrays[0].Set(rprintf("%llu",			paxosID));
	bytearrays[1].Set(rprintf("%d",				state.accepted));
	bytearrays[2].Set(rprintf("%llu",			state.promisedProposalID));
	bytearrays[3].Set(rprintf("%llu",			state.acceptedProposalID));
	
	ret = true;
	ret = ret && table->Set(transaction, "@@paxosID",				bytearrays[0]);
	ret = ret && table->Set(transaction, "@@accepted",				bytearrays[1]);
	ret = ret && table->Set(transaction, "@@promisedProposalID",	bytearrays[2]);
	ret = ret && table->Set(transaction, "@@acceptedProposalID",	bytearrays[3]);
	ret = ret && table->Set(transaction, "@@acceptedValue",			state.acceptedValue);
	
	if (!ret)
		return false;
	
	return true;
}

bool PaxosAcceptor::ReadState()
{
	Log_Trace();
	
	bool ret;
	int nread;
	
	if (table == NULL)
		return false;

	ret = true;
	ret = ret && table->Get(NULL, "@@paxosID",				bytearrays[0]);
	ret = ret && table->Get(NULL, "@@accepted",				bytearrays[1]);
	ret = ret && table->Get(NULL, "@@promisedProposalID",	bytearrays[2]);
	ret = ret && table->Get(NULL, "@@acceptedProposalID",	bytearrays[3]);
	ret = ret && table->Get(NULL, "@@acceptedValue",		state.acceptedValue);

	if (!ret)
		return false;

	paxosID = strntoulong64(bytearrays[0].buffer, bytearrays[0].length, &nread);
	if (nread != bytearrays[0].length)
	{
		Log_Trace();
		return false;
	}
	
	state.accepted = strntolong64(bytearrays[1].buffer, bytearrays[1].length, &nread);
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

bool PaxosAcceptor::WriteState()
{
	Log_Trace();

	bool ret;
	
	if (table == NULL)
		return false;
	
	writtenPaxosID = paxosID;
	
	bytearrays[0].Set(rprintf("%llu",			paxosID));
	bytearrays[1].Set(rprintf("%d",				state.accepted));
	bytearrays[2].Set(rprintf("%llu",			state.promisedProposalID));
	bytearrays[3].Set(rprintf("%llu",			state.acceptedProposalID));
	
	mdbop.Init();
	
	ret = true;
	ret = ret && mdbop.Set(table, "@@paxosID",				bytearrays[0]);
	ret = ret && mdbop.Set(table, "@@accepted",				bytearrays[1]);
	ret = ret && mdbop.Set(table, "@@promisedProposalID",	bytearrays[2]);
	ret = ret && mdbop.Set(table, "@@acceptedProposalID",	bytearrays[3]);
	ret = ret && mdbop.Set(table, "@@acceptedValue",		state.acceptedValue);

	if (!ret)
		return false;
	
	mdbop.SetTransaction(&transaction);
	
	dbWriter.Add(&mdbop);

	return true;
}

void PaxosAcceptor::SendReply(unsigned nodeID)
{
	msg.Write(wdata);

	writers[nodeID]->Write(wdata);
}

void PaxosAcceptor::OnPrepareRequest(PaxosMsg& msg_)
{
	Log_Trace();
	
	if (mdbop.active)
		return;
	
	msg = msg_;
	
	senderID = msg.nodeID;
	
	if (msg.proposalID < state.promisedProposalID)
	{
		msg.PrepareResponse(msg.paxosID, PaxosConfig::Get()->nodeID, msg.proposalID, PREPARE_REJECTED);
		
		SendReply(senderID);
		return;
	}

	state.promisedProposalID = msg.proposalID;

	if (!state.accepted)
		msg.PrepareResponse(msg.paxosID, PaxosConfig::Get()->nodeID,
			msg.proposalID, PREPARE_CURRENTLY_OPEN);
	else
		msg.PrepareResponse(msg.paxosID, PaxosConfig::Get()->nodeID,
			msg.proposalID, PREPARE_PREVIOUSLY_ACCEPTED, state.acceptedProposalID, state.acceptedValue);
	
	WriteState();
}

void PaxosAcceptor::OnProposeRequest(PaxosMsg& msg_)
{
	Log_Trace();
	
	if (mdbop.active)
		return;
	
	msg = msg_;
	
	senderID = msg.nodeID;
	
	if (msg.proposalID < state.promisedProposalID)
	{
		msg.ProposeResponse(msg.paxosID, PaxosConfig::Get()->nodeID, msg.proposalID, PROPOSE_REJECTED);
		
		SendReply(senderID);
		return;
	}

	state.accepted = true;
	state.acceptedProposalID = msg.proposalID;
	if (!state.acceptedValue.Set(msg.value))
		ASSERT_FAIL();
	msg.ProposeResponse(msg.paxosID, PaxosConfig::Get()->nodeID, msg.proposalID, PROPOSE_ACCEPTED);
	
	WriteState();
}

void PaxosAcceptor::OnDBComplete()
{
	Log_Trace();

	if (writtenPaxosID == paxosID)
		SendReply(senderID); // TODO: check that the transaction commited
}
