#include "Paxos.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "Framework/Database/Transaction.h"

Paxos::Paxos() :
	onRead(this, &Paxos::OnRead),
	onWrite(this, &Paxos::OnWrite),
	onPrepareTimeout(this, &Paxos::OnPrepareTimeout),
	onProposeTimeout(this, &Paxos::OnProposeTimeout),
	prepareTimeout(PAXOS_TIMEOUT, &onPrepareTimeout),
	proposeTimeout(PAXOS_TIMEOUT, &onProposeTimeout),
	onDBComplete(this, &Paxos::OnDBComplete)
{
	mdbop.SetCallback(&onDBComplete);
}

bool Paxos::Start(ByteString value)
{
	if (!proposer.value.Set(value))
		return false;
	
	if (!proposer.leader)
		Prepare();
	else
		Propose(); // multi paxos, skip prepare
	
	return true;
}

bool Paxos::Init(IOProcessor* ioproc_, Scheduler* scheduler_)
{
	// I/O framework
	ioproc = ioproc_;
	scheduler = scheduler_;
	
	table = database.GetTable("state");
	if (table == NULL)
	{
		Log_Trace();
		return false;
	}
	transaction.Set(table);

	// Paxos variables
	paxosID = 0;
	Reset();
	proposer.leader = false;

	//if (!ReadState())
	{
		Log_Message("*** Paxos is starting from scratch (ReadState() failed) *** ");
	}
	
	if (!socket.Create(UDP)) return false;
	if (!socket.Bind(config.port)) return false;
	if (!socket.SetNonblocking()) return false;
	
	udpread.fd = socket.fd;
	udpread.data = rdata;
	udpread.onComplete = &onRead;
	
	udpwrite.fd = socket.fd;
	udpwrite.data = wdata;
	udpwrite.onComplete = &onWrite;
	
	return ioproc->Add(&udpread);
}

bool Paxos::ReadState()
{
	Log_Trace();
	
	bool ret;
	int nread;
	
	if (table == NULL)
		return false;

	ret = true;
	ret = ret && table->Get(NULL, "paxosID",			bytearrays[0]);
	ret = ret && table->Get(NULL, "accepted",			bytearrays[1]);
	ret = ret && table->Get(NULL, "n_highest_promised",	bytearrays[2]);
	ret = ret && table->Get(NULL, "n_accepted",			bytearrays[3]);
	ret = ret && table->Get(NULL, "accepted",			acceptor.value);

	if (!ret)
		return false;

	paxosID = strntoulong64(bytearrays[0].buffer, bytearrays[0].length, &nread);
	if (nread != bytearrays[0].length)
	{
		Log_Trace();
		return false;
	}
	
	acceptor.accepted = strntol(bytearrays[1].buffer, bytearrays[1].length, &nread);
	if (nread != bytearrays[1].length)
	{
		Log_Trace();
		return false;
	}
	
	acceptor.n_highest_promised = strntoulong64(bytearrays[2].buffer, bytearrays[2].length, &nread);
	if (nread != bytearrays[2].length)
	{
		Log_Trace();
		return false;
	}
	
	acceptor.n_accepted = strntoulong64(bytearrays[3].buffer, bytearrays[3].length, &nread);
	if (nread != bytearrays[3].length)
	{
		Log_Trace();
		return false;
	}
	
	return true;
}

bool Paxos::WriteState(Transaction* transaction)
{
	Log_Trace();

	bool ret;
	
	if (table == NULL)
		return false;
	
	bytearrays[0].Set(rprintf("%llu",			paxosID));
	bytearrays[1].Set(rprintf("%d",				acceptor.accepted));
	bytearrays[2].Set(rprintf("%llu",			acceptor.n_highest_promised));
	bytearrays[3].Set(rprintf("%llu",			acceptor.n_accepted));
	
	mdbop.Init();
	
	ret = true;
	ret = ret && mdbop.Set(table, "paxosID",				bytearrays[0]);
	ret = ret && mdbop.Set(table, "accepted",			bytearrays[1]);
	ret = ret && mdbop.Set(table, "n_highest_promised",	bytearrays[2]);
	ret = ret && mdbop.Set(table, "n_accepted",			bytearrays[3]);
	ret = ret && mdbop.Set(table, "value",				acceptor.value);

	if (!ret)
		return false;
	
	mdbop.SetTransaction(transaction);
	
	dbWriter.Add(&mdbop);

	return true;
}


bool Paxos::SendMsg()
{
	Log_Trace();
		
	if (!msg.Write(udpwrite.data))
		return false;
	
	if (udpwrite.data.length > 0)
	{
		Log_Message("Participant %d sending message %.*s to %s",
			config.nodeID, udpwrite.data.length, udpwrite.data.buffer,
			udpwrite.endpoint.ToString());
		
		ioproc->Add(&udpwrite);
		
		return true;
	}
	
	return false;
}

void Paxos::SendMsgToAll()
{
	Log_Trace();

	// remove outstanding read
	if (udpread.active)
		ioproc->Remove(&udpread);

	numSent		= 0;
	numReceived = 0;
	numAccepted = 0;
	numRejected = 0;

	sending_to = config.endpoints.Head();
	if (sending_to != NULL)
	{
		sendtoall = true;
		udpwrite.endpoint = *sending_to;
		SendMsg();
	}
}

void Paxos::Reset()
{
	proposer.Reset();
	acceptor.Reset();

	StopPreparer();
	StopProposer();

	sendtoall = false;
}

void Paxos::OnRead()
{
	Log_Trace();
		
	Log_Message("Participant %d received message %.*s from %s",
		config.nodeID, udpread.data.length, udpread.data.buffer, udpread.endpoint.ToString());
	
	if (!msg.Read(udpread.data))
	{
		Log_Message("PaxosMsg::Read() failed: invalid message format");
		ioproc->Add(&udpread); // read again
	} else
		ProcessMsg();
	
	assert(EXACTLY_ONE(udpread.active, udpwrite.active, mdbop.active));
}

void Paxos::OnWrite()
{
	Log_Trace();

	if (sendtoall)
	{
		numSent++;
		sending_to = config.endpoints.Next(sending_to);
		if (sending_to != NULL)
		{
			udpwrite.endpoint = *sending_to;

			Log_Message("Participant %d sending message %.*s to %s",
				config.nodeID, udpwrite.data.length, udpwrite.data.buffer,
				udpwrite.endpoint.ToString());

			ioproc->Add(&udpwrite);
		}
		else
			sendtoall = false;
	}

	if (!sendtoall)
	{
		Log_Trace();
		ioproc->Add(&udpread);
	}
	
	assert(EXACTLY_ONE(udpread.active, udpwrite.active, mdbop.active));
}

void Paxos::ProcessMsg()
{
	if (msg.type == PREPARE_REQUEST)
		OnPrepareRequest();
	else if (msg.type == PREPARE_RESPONSE)
		OnPrepareResponse();
	else if (msg.type == PROPOSE_REQUEST)
		OnProposeRequest();
	else if (msg.type == PROPOSE_RESPONSE)
		OnProposeResponse();
	else if (msg.type == LEARN_CHOSEN)
		OnLearnChosen();
	else
		ASSERT_FAIL();
}

void Paxos::OnPrepareRequest()
{
	Log_Trace();
	
	if (acceptor.learned)
	{
		if (!msg.LearnChosen(msg.paxosID, acceptor.value))
		{
			Log_Trace();
			ioproc->Add(&udpread);
			return;
		}
		
		udpwrite.endpoint = udpread.endpoint;
		if (!msg.Write(udpwrite.data))
		{
			Log_Trace();
			ioproc->Add(&udpread);
			return;
		}
		
		SendMsg();
		return;
	}

	/*	Lamport: If an acceptor receives a prepare request with number n greater 
		than that of any prepare request to which it has already responded, 
		then it responds to the request with a promise not to accept any more 
		proposals numbered less than n and with the highest-numbered pro- 
		posal (if any) that it has accepted.
	*/
	
	if (msg.n < acceptor.n_highest_promised)
	{
		msg.PrepareResponse(msg.paxosID, msg.n, PREPARE_REJECTED);
		
		udpwrite.endpoint = udpread.endpoint;
		if (!msg.Write(udpwrite.data))
		{
			Log_Trace();
			ioproc->Add(&udpread);
			return;
		}
		
		SendMsg();
		return;
	}

	acceptor.n_highest_promised = msg.n;

	if (!acceptor.accepted)
		msg.PrepareResponse(msg.paxosID, msg.n, PREPARE_CURRENTLY_OPEN);
	else
		msg.PrepareResponse(msg.paxosID, msg.n, PREPARE_PREVIOUSLY_ACCEPTED,
			acceptor.n_accepted, acceptor.value);
	
	WriteState(&transaction);
}

void Paxos::OnPrepareResponse()
{
	Log_Trace();

	/*	Lamport: If the proposer receives a response to its prepare requests
		(numbered n) from a majority of acceptors, then it sends an accept (propose)
		request to each of those acceptors for a proposal numbered n with a 
		value v, where v is the value of the highest-numbered proposal among 
		the responses, or is any value if the responses reported no proposals.
	*/
	
	if (!proposer.preparing || msg.n != proposer.n_proposing)
	{
		ioproc->Add(&udpread);
		return;
	}
		
	numReceived++;
	
	if (msg.response == PREPARE_REJECTED)
		numRejected++;
	else if (msg.response == PREPARE_PREVIOUSLY_ACCEPTED)
	{
		if (msg.n_accepted >= proposer.n_highest_received)
		{
			/* the >= could be replaced by > which would result in less copys
			 * however this would result in complications in multi paxos
			 * in the multi paxos steady state this branch is inactive
			 * it only runs after leader failure
			 * so it's ok
			 */
			proposer.n_highest_received = msg.n_accepted;
			if (!proposer.value.Set(msg.value))
				ASSERT_FAIL();
		}
	}
	else
		proposer.numOpen++;
	
	bool read = TryFinishPrepare();
		
	if (read)
		ioproc->Add(&udpread);
}

void Paxos::OnProposeRequest()
{
	Log_Trace();
	
	if (acceptor.learned)
	{
		msg.LearnChosen(msg.paxosID, acceptor.value);
		udpwrite.endpoint = udpread.endpoint;
		if (!msg.Write(udpwrite.data))
		{
			Log_Trace();
			ioproc->Add(&udpread);
			return;
		}
		
		SendMsg();
		return;
	}

	/*	Lamport: An acceptor can accept a proposal numbered n iﬀ it has not responded 
		to a prepare request having a number greater than n .
		
		Lamport: If an acceptor receives an accept request for a proposal numbered n,
		it accepts the proposal unless it has already responded to a prepare 
		request having a number greater than n .
	*/
	if (msg.n < acceptor.n_highest_promised)
	{
		msg.ProposeResponse(msg.paxosID, msg.n, PROPOSE_REJECTED);
		
		udpwrite.endpoint = udpread.endpoint;
		if (!msg.Write(udpwrite.data))
		{
			Log_Trace();
			ioproc->Add(&udpread);
			return;
		}
		
		SendMsg();
		return;
	}

	acceptor.accepted = true;
	acceptor.n_accepted = msg.n;
	if (!acceptor.value.Set(msg.value))
		ASSERT_FAIL();
	msg.ProposeResponse(msg.paxosID, msg.n, PROPOSE_ACCEPTED);
	
	WriteState(&transaction);	
}

void Paxos::OnProposeResponse()
{
	Log_Trace();
	
	if (!proposer.proposing || msg.n != proposer.n_proposing)
	{
		ioproc->Add(&udpread);
		return;
	}
	
	numReceived++;
	
	if (msg.response == PROPOSE_ACCEPTED)
		numAccepted++;

	bool read = TryFinishPropose();
		
	if (read)
		ioproc->Add(&udpread);
}

void Paxos::OnLearnChosen()
{
	Log_Trace();
		
	acceptor.learned = true;
	if (!acceptor.value.Set(msg.value))
		ASSERT_FAIL();

	Log_Message("*** Node %d: consensus for paxosID = %d is: %.*s ***",
				config.nodeID, paxosID, acceptor.value.length, acceptor.value.buffer);
}

void Paxos::StopPreparer()
{
	Log_Trace();

	proposer.preparing = false;
	scheduler->Remove(&prepareTimeout);
}

void Paxos::StopProposer()
{
	Log_Trace();
	
	proposer.proposing = false;
	scheduler->Remove(&proposeTimeout);
}

void Paxos::Prepare()
{
	Log_Trace();

	StopProposer();
	proposer.preparing = true;
	
	if (!(proposer.leader && acceptor.n_highest_promised == 0))
		proposer.n_proposing = config.NextHighest(acceptor.n_highest_promised);
	
	proposer.n_highest_received = 0; // TODO: should be -1 ?
	proposer.numOpen = 0;
	
	msg.PrepareRequest(paxosID, proposer.n_proposing);
	
	SendMsgToAll();
	
	scheduler->Reset(&prepareTimeout);
}

// returns whether to put out an IORead
bool Paxos::TryFinishPrepare()
{
	Log_Trace();
	
	/*	Lamport: If the proposer receives a response to its prepare requests
		(numbered n) from a majority of acceptors, then it sends an accept (propose)
		request to each of those acceptors for a proposal numbered n with a 
		value v, where v is the value of the highest-numbered proposal among 
		the responses, or is any value if the responses reported no proposals.
	*/
		
	if (proposer.n_proposing < acceptor.n_highest_promised ||
			numRejected >= ceil(config.numNodes / 2))
	{
		Prepare();
		return false;
	}
	
	// see if we have enough positive replies to advance	
	int numPositive = numReceived - numRejected;
	if (numPositive >= config.MinMajority())
	{
		if (proposer.numOpen == numPositive)
		{
			// free to propose my value, stored in proposer, set by the application
			Propose();
			return false;
		}
		else
		{
			Propose();
			return false;
		}
	}
	
	if (numReceived == numSent)
	{
		Prepare();
		return false;
	}
	
	return true;
}

void Paxos::Propose()
{
	Log_Trace();
	
	StopPreparer();

	proposer.proposing = true;

	msg.ProposeRequest(paxosID, proposer.n_proposing, proposer.value);
	SendMsgToAll();
	
	scheduler->Reset(&proposeTimeout);
}

// returns whether to put out an IORead
bool Paxos::TryFinishPropose()
{
	Log_Trace();
	
	// see if we have enough positive replies to advance
	if (numAccepted >= config.MinMajority())
	{
		// a majority have accepted our proposal, we have consensus
		StopProposer();
		msg.LearnChosen(paxosID, proposer.value);
		SendMsgToAll();
		return false;
	}
	
	if (numReceived == numSent)
	{
		Prepare();
		return false;
	}
	
	return true;
}

void Paxos::OnPrepareTimeout()
{
	Log_Trace();
	
	assert(proposer.preparing);
	
	if (mdbop.active)
	{
		Log_Message("Waiting for dbop to complete...");
		scheduler->Reset(&prepareTimeout);
		return;
	}
	
	Prepare();
}

void Paxos::OnProposeTimeout()
{
	Log_Trace();
	
	assert(proposer.proposing);
	
	if (mdbop.active)
	{
		Log_Message("Waiting for dbop to complete...");
		scheduler->Reset(&proposeTimeout);
		return;
	}

	
	Prepare();
}

void Paxos::OnDBComplete()
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
	
	SendMsg();
	
	assert(EXACTLY_ONE(udpread.active, udpwrite.active, mdbop.active));
}
