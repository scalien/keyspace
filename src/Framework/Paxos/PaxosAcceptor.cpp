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

void PaxosAcceptor::Init(Writers writers_)
{
	writers = writers_;
	
	table = database.GetTable("keyspace");
	if (table == NULL)
		ASSERT_FAIL();
	transaction.Set(table);

	paxosID = 0;
	state.Init();

	if (!ReadState())
		Log_Message("*** No Keyspace database found. "
					"Starting from scratch... *** ");
}

void PaxosAcceptor::Shutdown()
{
	if (transaction.IsActive())
		transaction.Rollback();
}

bool PaxosAcceptor::Persist(Transaction* transaction)
{
	Log_Trace();

	bool ret;
	
	if (table == NULL)
		return false;
	
	ret = true;
	ret &= table->Set(transaction,
					  "@@paxosID",
					  rprintf("%" PRIu64 "", paxosID));
					  
	ret &= table->Set(transaction,
					  "@@accepted",
					  rprintf("%d", state.accepted));
	
	ret &= table->Set(transaction,
					  "@@promisedProposalID",
					  rprintf("%" PRIu64 "", state.promisedProposalID));
					  
	ret &= table->Set(transaction,
					  "@@acceptedProposalID",
					  rprintf("%" PRIu64 "", state.acceptedProposalID));
					  
	ret &= table->Set(transaction,
					  "@@acceptedValue",
					  state.acceptedValue);
	
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

	ret = table->Get(NULL, "@@paxosID", buffers[0]);
	if (!ret)
		return false;

	nread = 0;
	paxosID = strntouint64(buffers[0].buffer, buffers[0].length, &nread);
	if (nread != (unsigned) buffers[0].length)
	{
		Log_Trace();
		paxosID = 0;
		return false;
	}

	ret = table->Get(NULL, "@@accepted", buffers[1]);
	if (!ret)
	{
		// going from single to multi config
		state.Init();
		return (paxosID > 0);
	}
	
	nread = 0;
	state.accepted =
		strntoint64(buffers[1].buffer, buffers[1].length, &nread) ? true : false;
	if (nread != (unsigned) buffers[1].length)
	{
		state.accepted = false;
		Log_Trace();
		return false;
	}

	ret = table->Get(NULL, "@@promisedProposalID", buffers[2]);
	if (!ret)
		return false;

	nread = 0;
	state.promisedProposalID =
		strntouint64(buffers[2].buffer, buffers[2].length, &nread);
	if (nread != (unsigned) buffers[2].length)
	{
		state.promisedProposalID = 0;
		Log_Trace();
		return false;
	}

	ret = table->Get(NULL, "@@acceptedProposalID", buffers[3]);
	if (!ret)
		return false;

	nread = 0;
	state.acceptedProposalID =
		strntouint64(buffers[3].buffer, buffers[3].length, &nread);
	if (nread != (unsigned) buffers[3].length)
	{
		state.acceptedProposalID = 0;
		Log_Trace();
		return false;
	}

	ret = table->Get(NULL, "@@acceptedValue", state.acceptedValue);
	
	return (paxosID > 0);
}

bool PaxosAcceptor::WriteState()
{
	Log_Trace();

	bool ret;
	
	if (table == NULL)
		return false;
	
	writtenPaxosID = paxosID;
	
	buffers[0].Set(rprintf("%" PRIu64 "",	paxosID));
	buffers[1].Set(rprintf("%d",			state.accepted));
	buffers[2].Set(rprintf("%" PRIu64 "",	state.promisedProposalID));
	buffers[3].Set(rprintf("%" PRIu64 "",	state.acceptedProposalID));
	
	mdbop.Init();
	mdbop.SetCallback(&onDBComplete);
	
	ret = true;
	ret &= mdbop.Set(table, "@@paxosID",			buffers[0]);
	ret &= mdbop.Set(table, "@@accepted",			buffers[1]);
	ret &= mdbop.Set(table, "@@promisedProposalID",	buffers[2]);
	ret &= mdbop.Set(table, "@@acceptedProposalID",	buffers[3]);
	ret &= mdbop.Set(table, "@@acceptedValue",		state.acceptedValue);

	if (!ret)
		return false;
	
	mdbop.SetTransaction(&transaction);
	
	dbWriter.Add(&mdbop);

	return true;
}

void PaxosAcceptor::SendReply(unsigned nodeID)
{
	msg.Write(msgbuf);

	writers[nodeID]->Write(msgbuf);
}

void PaxosAcceptor::OnPrepareRequest(PaxosMsg& msg_)
{
	Log_Trace();
	
	if (mdbop.IsActive())
		return;
	
	msg = msg_;
	
	senderID = msg.nodeID;
	
	Log_Trace("state.promisedProposasID: %" PRIu64 ""
			   "msg.proposalID: %" PRIu64 "",
			   state.promisedProposalID, msg.proposalID);
	
	if (msg.proposalID < state.promisedProposalID)
	{
		msg.PrepareRejected(msg.paxosID,
			RCONF->GetNodeID(),
			msg.proposalID,
			state.promisedProposalID);
		
		SendReply(senderID);
		return;
	}

	state.promisedProposalID = msg.proposalID;

	if (!state.accepted)
		msg.PrepareCurrentlyOpen(msg.paxosID,
			RCONF->GetNodeID(),
			msg.proposalID);
	else
		msg.PreparePreviouslyAccepted(msg.paxosID,
			RCONF->GetNodeID(),
			msg.proposalID,
			state.acceptedProposalID,
			state.acceptedValue);
	
	WriteState();
	RLOG->StopPaxos();
	RLOG->StopReplicatedDB();
}

void PaxosAcceptor::OnProposeRequest(PaxosMsg& msg_)
{
	Log_Trace();
	
	if (mdbop.IsActive())
		return;
	
	msg = msg_;
	
	senderID = msg.nodeID;

	Log_Trace("state.promisedProposasID: %" PRIu64 ""
				"msg.proposalID: %" PRIu64 "",
				state.promisedProposalID, msg.proposalID);
	
	if (msg.proposalID < state.promisedProposalID)
	{
		msg.ProposeRejected(msg.paxosID,
			RCONF->GetNodeID(),
			msg.proposalID);
		
		SendReply(senderID);
		return;
	}

	state.accepted = true;
	state.acceptedProposalID = msg.proposalID;
	if (!state.acceptedValue.Set(msg.value))
		ASSERT_FAIL();
	msg.ProposeAccepted(msg.paxosID,
		RCONF->GetNodeID(),
		msg.proposalID);
	
	WriteState();
	RLOG->StopPaxos();
}

void PaxosAcceptor::OnDBComplete()
{
	Log_Trace();

	RLOG->ContinuePaxos();
	RLOG->ContinueReplicatedDB();

	if (writtenPaxosID == paxosID)
		SendReply(senderID); // TODO: check that the transaction commited
}
