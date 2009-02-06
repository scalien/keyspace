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
	proposeTimeout(PAXOS_TIMEOUT, &onProposeTimeout)
{
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

	// Paxos variables
	paxosID = 0; //TODO: read from persistent storage
	Reset();
	proposer.leader = false;
		
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

bool Paxos::PersistState(Transaction* transaction)
{
/*
	if (table == NULL)
		return false;

	// TODO: check return values
	
	table->Put(transaction, "paxosID", rprintf("%llu", paxosID));
	table->Put(transaction, "accepted", rprintf("%d", acceptor.accepted));
	table->Put(transaction, "n_highest_promised", rprintf("%llu", acceptor.n_highest_promised));
	table->Put(transaction, "n_accepted", rprintf("%llu", acceptor.n_accepted));
	table->Put(transaction, "value", acceptor.value);
*/
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
		
	assert((udpread.active || udpwrite.active) && !(udpread.active && udpwrite.active));
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
	
	assert((udpread.active || udpwrite.active) && !(udpread.active && udpwrite.active));
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
	}
	else
	{
		/*	Lamport: If an acceptor receives a prepare request with number n greater 
			than that of any prepare request to which it has already responded, 
			then it responds to the request with a promise not to accept any more 
			proposals numbered less than n and with the highest-numbered pro- 
			posal (if any) that it has accepted.
		*/
		if (msg.n < acceptor.n_highest_promised)
			msg.PrepareResponse(msg.paxosID, msg.n, PREPARE_REJECTED);
		else
		{
			acceptor.n_highest_promised = msg.n;
		
			if (!acceptor.accepted)
				msg.PrepareResponse(msg.paxosID, msg.n, PREPARE_CURRENTLY_OPEN);
			else
				msg.PrepareResponse(msg.paxosID, msg.n, PREPARE_PREVIOUSLY_ACCEPTED,
					acceptor.n_accepted, acceptor.value);
		}
	}
	
	udpwrite.endpoint = udpread.endpoint;
	if (!msg.Write(udpwrite.data))
	{
		Log_Trace();
		ioproc->Add(&udpread);
		return;
	}
	
	SendMsg();
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
		msg.LearnChosen(msg.paxosID, acceptor.value);
	else
	{
		/*	Lamport: An acceptor can accept a proposal numbered n iﬀ it has not responded 
			to a prepare request having a number greater than n .
			
			Lamport: If an acceptor receives an accept request for a proposal numbered n,
			it accepts the proposal unless it has already responded to a prepare 
			request having a number greater than n .
		*/
		if (msg.n < acceptor.n_highest_promised)
			msg.ProposeResponse(msg.paxosID, msg.n, PROPOSE_REJECTED);
		else
		{
			acceptor.accepted = true;
			acceptor.n_accepted = msg.n;
			if (!acceptor.value.Set(msg.value))
				ASSERT_FAIL();
			msg.ProposeResponse(msg.paxosID, msg.n, PROPOSE_ACCEPTED);
		}
	}
	
	udpwrite.endpoint = udpread.endpoint;
	if (!msg.Write(udpwrite.data))
	{
		Log_Trace();
		ioproc->Add(&udpread);
		return;
	}
	
	SendMsg();
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
	
	Prepare();
}

void Paxos::OnProposeTimeout()
{
	Log_Trace();
	
	assert(proposer.proposing);
	
	Prepare();
}
