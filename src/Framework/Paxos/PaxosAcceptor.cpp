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
	
	ret = true;
	ret &= table->Set(transaction, "@@paxosID", rprintf("%" PRIu64 "", paxosID));
	ret &= table->Set(transaction, "@@accepted", rprintf("%d", state.accepted));
	ret &= table->Set(transaction, "@@promisedProposalID", rprintf("%" PRIu64 "", state.promisedProposalID));
	ret &= table->Set(transaction, "@@acceptedProposalID", rprintf("%" PRIu64 "", state.acceptedProposalID));
	ret &= table->Set(transaction, "@@acceptedValue",	state.acceptedValue);
	
	if (!ret)
		return false;
	
	return true;
}

bool PaxosAcceptor::ReadState()
{
	Log_Trace();
	
	bool ret;
	unsigned nread;
	
	if (table == NULL)
		return false;

	ret = true;
	ret &= table->Get(NULL, "@@paxosID",			data[0]);
	ret &= table->Get(NULL, "@@accepted",			data[1]);
	ret &= table->Get(NULL, "@@promisedProposalID",	data[2]);
	ret &= table->Get(NULL, "@@acceptedProposalID",	data[3]);
	ret &= table->Get(NULL, "@@acceptedValue",		state.acceptedValue);

	if (!ret)
		return false;

	paxosID = strntouint64_t(data[0].buffer, data[0].length, &nread);
	if (nread != (unsigned) data[0].length)
	{
		Log_Trace();
		return false;
	}
	
	state.accepted = strntoint64_t(data[1].buffer, data[1].length, &nread);
	if (nread != (unsigned) data[1].length)
	{
		Log_Trace();
		return false;
	}
	
	state.promisedProposalID = strntouint64_t(data[2].buffer, data[2].length, &nread);
	if (nread != (unsigned) data[2].length)
	{
		Log_Trace();
		return false;
	}
	
	state.acceptedProposalID = strntouint64_t(data[3].buffer, data[3].length, &nread);
	if (nread != (unsigned) data[3].length)
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
	
	data[0].Set(rprintf("%" PRIu64 "",	paxosID));
	data[1].Set(rprintf("%d",			state.accepted));
	data[2].Set(rprintf("%" PRIu64 "",	state.promisedProposalID));
	data[3].Set(rprintf("%" PRIu64 "",	state.acceptedProposalID));
	
	mdbop.Init();
	
	ret = true;
	ret &= mdbop.Set(table, "@@paxosID",			data[0]);
	ret &= mdbop.Set(table, "@@accepted",			data[1]);
	ret &= mdbop.Set(table, "@@promisedProposalID",	data[2]);
	ret &= mdbop.Set(table, "@@acceptedProposalID",	data[3]);
	ret &= mdbop.Set(table, "@@acceptedValue",		state.acceptedValue);

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
