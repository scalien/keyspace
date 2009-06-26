#include "PaxosAcceptor.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "System/Events/EventLoop.h"
#include "Framework/Database/Transaction.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"

PaxosAcceptor::PaxosAcceptor() :
	onDBComplete(this, &PaxosAcceptor::OnDBComplete)
{
}

void PaxosAcceptor::Init(TransportWriter** writers_)
{
	writers = writers_;
	
	table = database.GetTable("keyspace");
	if (table == NULL)
		ASSERT_FAIL();
	transaction.Set(table);

	paxosID = 0;
	state.Init();

	if (!ReadState())
		Log_Message("*** No Keyspace database found. Starting from scratch... *** ");
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
	unsigned nread = 0;
	
	if (table == NULL)
		return false;
		
	state.acceptedValue.Allocate(2*MB + 10*KB);

	ret = true;
	ret &= table->Get(NULL, "@@paxosID",			data[0]);
	ret &= table->Get(NULL, "@@accepted",			data[1]);
	ret &= table->Get(NULL, "@@promisedProposalID",	data[2]);
	ret &= table->Get(NULL, "@@acceptedProposalID",	data[3]);
	ret &= table->Get(NULL, "@@acceptedValue",		state.acceptedValue);

	if (!ret)
		return false;

	nread = 0;
	paxosID = strntouint64(data[0].buffer, data[0].length, &nread);
	if (nread != (unsigned) data[0].length)
	{
		Log_Trace();
		return false;
	}
	
	nread = 0;
	state.accepted = strntoint64(data[1].buffer, data[1].length, &nread);
	if (nread != (unsigned) data[1].length)
	{
		Log_Trace();
		return false;
	}
	
	nread = 0;
	state.promisedProposalID = strntouint64(data[2].buffer, data[2].length, &nread);
	if (nread != (unsigned) data[2].length)
	{
		Log_Trace();
		return false;
	}
	
	nread = 0;
	state.acceptedProposalID = strntouint64(data[3].buffer, data[3].length, &nread);
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
	mdbop.SetCallback(&onDBComplete);
	
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
	
	if (mdbop.IsActive())
		return;
	
	msg = msg_;
	
	senderID = msg.nodeID;
	
	Log_Trace("state.promisedProposasID: %" PRIu64 " msg.proposalID: %" PRIu64 "",
				state.promisedProposalID, msg.proposalID);
	
	if (msg.proposalID < state.promisedProposalID)
	{
		msg.PrepareRejected(msg.paxosID, ReplicatedConfig::Get()->nodeID, msg.proposalID,
			state.promisedProposalID);
		
		SendReply(senderID);
		return;
	}

	state.promisedProposalID = msg.proposalID;

	if (!state.accepted)
		msg.PrepareCurrentlyOpen(msg.paxosID, ReplicatedConfig::Get()->nodeID,
			msg.proposalID);
	else
		msg.PreparePreviouslyAccepted(msg.paxosID, ReplicatedConfig::Get()->nodeID,
			msg.proposalID, state.acceptedProposalID, state.acceptedValue);
	
	WriteState();
	ReplicatedLog::Get()->StopPaxos();
}

void PaxosAcceptor::OnProposeRequest(PaxosMsg& msg_)
{
	Log_Trace();
	
	if (mdbop.IsActive())
		return;
	
	msg = msg_;
	
	senderID = msg.nodeID;

	Log_Trace("state.promisedProposasID: %" PRIu64 " msg.proposalID: %" PRIu64 "",
				state.promisedProposalID, msg.proposalID);
	
	if (msg.proposalID < state.promisedProposalID)
	{
		msg.ProposeRejected(msg.paxosID, ReplicatedConfig::Get()->nodeID, msg.proposalID);
		
		SendReply(senderID);
		return;
	}

	state.accepted = true;
	state.acceptedProposalID = msg.proposalID;
	if (!state.acceptedValue.Set(msg.value))
		ASSERT_FAIL();
	msg.ProposeAccepted(msg.paxosID, ReplicatedConfig::Get()->nodeID, msg.proposalID);
	
	WriteState();
	ReplicatedLog::Get()->StopPaxos();
}

void PaxosAcceptor::OnDBComplete()
{
	Log_Trace();

	ReplicatedLog::Get()->ContinuePaxos();

	if (writtenPaxosID == paxosID)
		SendReply(senderID); // TODO: check that the transaction commited
}
