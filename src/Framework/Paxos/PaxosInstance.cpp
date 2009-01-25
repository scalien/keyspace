#include "PaxosInstance.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"

PaxosInstance::PaxosInstance() :
	readCallable(this, &PaxosInstance::OnRead),
	writeCallable(this, &PaxosInstance::OnWrite),
	onPrepareTimeout(this, &PaxosInstance::OnPrepareTimeout),
	onProposeTimeout(this, &PaxosInstance::OnProposeTimeout),
	prepareTimeout(PAXOS_TIMEOUT, &onPrepareTimeout),
	proposeTimeout(PAXOS_TIMEOUT, &onProposeTimeout)
{
}

bool PaxosInstance::Start(ByteString value)
{
	if (!proposer.value.Set(value))
		return false;

	Prepare();
	
	return true;
}

bool PaxosInstance::Init(IOProcessor* ioproc_, Scheduler* scheduler_)
{
	// I/O framework
	ioproc = ioproc_;
	scheduler = scheduler_;

	// Paxos variables
	paxosID = 0;
	Reset();
		
	socket.Create(UDP);
	socket.Bind(config.port);
	socket.SetNonblocking();
	Log_Message("Bound to socket %d", socket.fd);
	
	udpread.fd = socket.fd;
	udpread.data = rdata;
	udpread.onComplete = &readCallable;
	
	udpwrite.fd = socket.fd;
	udpwrite.data = wdata;
	udpwrite.onComplete = &writeCallable;
	
	return ioproc->Add(&udpread);
}

bool PaxosInstance::SendMsg()
{
	Log_Trace();
		
	if (!PaxosMsg::Write(&msg, udpwrite.data))
		return false;
	
	if (udpwrite.data.length > 0)
	{
		udpwrite.data.buffer[udpwrite.data.length] = '\0'; // todo: remove
		Log_Message("Participant %d sending message %s to %s",
			config.nodeID, udpwrite.data.buffer, udpwrite.endpoint.ToString());
		
		ioproc->Add(&udpwrite);
		
		return true;
	}
	
	return false;
}

void PaxosInstance::SendMsgToAll()
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

void PaxosInstance::Reset()
{
	highest_promised = 0;
	preparer.n_highest_received = -1;
	StopPreparer();
	preparer.numOpen = 0;
	StopProposer();
	proposer.n = 0;
	acceptor.accepted = false;
	acceptor.n = 0;
	learner.learned = false;
	sendtoall = false;
}

void PaxosInstance::OnRead()
{
	Log_Trace();
	
	udpread.data.buffer[udpread.data.length] = '\0'; // todo: remove
	
	Log_Message("Participant %d received message %s from %s",
		config.nodeID, udpread.data.buffer, udpread.endpoint.ToString());
	
	if (!PaxosMsg::Read(udpread.data, &msg))
	{
		Log_Message("PaxosMsg::Read() failed: invalid message format");
		ioproc->Add(&udpread); // read again
	} else
		ProcessMsg();
}

void PaxosInstance::OnWrite()
{
	Log_Trace();

	if (sendtoall)
	{
		numSent++;
		sending_to = config.endpoints.Next(sending_to);
		if (sending_to != NULL)
		{
			udpwrite.endpoint = *sending_to;

			udpwrite.data.buffer[udpwrite.data.length] = '\0'; // todo: remove
			Log_Message("Participant %d sending message %s to %s",
				config.nodeID, udpwrite.data.buffer, udpwrite.endpoint.ToString());

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
}

void PaxosInstance::ProcessMsg()
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
}

void PaxosInstance::OnPrepareRequest()
{
	Log_Trace();
	
	if (learner.learned)
		msg.LearnChosen(msg.paxosID, learner.value); // todo: retval
	else
	{
		/*	Lamport: If an acceptor receives a prepare request with number n greater 
			than that of any prepare request to which it has already responded, 
			then it responds to the request with a promise not to accept any more 
			proposals numbered less than n and with the highest-numbered pro- 
			posal (if any) that it has accepted.
		*/
		if (msg.n < highest_promised)
			msg.PrepareResponse(msg.paxosID, msg.n, PREPARE_REJECTED);
		else
		{
			highest_promised = msg.n;
		
			if (!acceptor.accepted)
				msg.PrepareResponse(msg.paxosID, msg.n, PREPARE_CURRENTLY_OPEN);
			else
				msg.PrepareResponse(msg.paxosID, msg.n, PREPARE_PREVIOUSLY_ACCEPTED,
					acceptor.n, acceptor.value);
		}
	}
	
	udpwrite.endpoint = udpread.endpoint;
	PaxosMsg::Write(&msg, udpwrite.data); // todo: retval
	SendMsg();
}

void PaxosInstance::OnPrepareResponse()
{
	Log_Trace();

	/*	Lamport: If the proposer receives a response to its prepare requests
		(numbered n) from a majority of acceptors, then it sends an accept (propose)
		request to each of those acceptors for a proposal numbered n with a 
		value v, where v is the value of the highest-numbered proposal among 
		the responses, or is any value if the responses reported no proposals.
	*/
	
	if (!preparer.active || msg.n != preparer.n)
	{
		ioproc->Add(&udpread);
		return;
	}
		
	numReceived++;
	
	if (msg.response == PREPARE_REJECTED)
		numRejected++;
	else if (msg.response == PREPARE_PREVIOUSLY_ACCEPTED)
	{
		if (msg.n_accepted > preparer.n_highest_received)
		{
			preparer.n_highest_received = msg.n_accepted;
			preparer.value.Set(msg.value); // todo: retval
		}
	}
	else
		preparer.numOpen++;
	
	bool read = TryFinishPrepare();
		
	if (read)
		ioproc->Add(&udpread);
}

void PaxosInstance::OnProposeRequest()
{
	Log_Trace();
	
	if (learner.learned)
		msg.LearnChosen(msg.paxosID, learner.value);
	else
	{
		/*	Lamport: An acceptor can accept a proposal numbered n iﬀ it has not responded 
			to a prepare request having a number greater than n .
			
			Lamport: If an acceptor receives an accept request for a proposal numbered n,
			it accepts the proposal unless it has already responded to a prepare 
			request having a number greater than n .
		*/
		if (msg.n < highest_promised)
			msg.ProposeResponse(msg.paxosID, msg.n, PROPOSE_REJECTED);
		else
		{
			acceptor.accepted = true;
			acceptor.n = msg.n;
			acceptor.value.Set(msg.value); // todo: retval
			msg.ProposeResponse(msg.paxosID, msg.n, PROPOSE_ACCEPTED);
		}
	}
	
	udpwrite.endpoint = udpread.endpoint;
	PaxosMsg::Write(&msg, udpwrite.data); // todo: retval
	SendMsg();
}

void PaxosInstance::OnProposeResponse()
{
	Log_Trace();
	
	if (!proposer.active || msg.n != proposer.n)
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

void PaxosInstance::OnLearnChosen()
{
	Log_Trace();
		
	learner.learned = true;
	learner.value.Set(msg.value); // todo: retval

	learner.value.buffer[learner.value.length] = '\0'; // todo: remove
	Log_Message("*** Pariticipart %d says consensus for paxosID = %d is: %s ***",
				config.nodeID, paxosID, learner.value.buffer);
}

void PaxosInstance::StopPreparer()
{
	Log_Trace();

	preparer.active = false;
	scheduler->Remove(&prepareTimeout);
}

void PaxosInstance::StopProposer()
{
	Log_Trace();
	
	proposer.active = false;
	scheduler->Remove(&proposeTimeout);
}

void PaxosInstance::Prepare()
{
	Log_Trace();

	StopProposer();
	preparer.active = true;
	
	preparer.n = config.NextHighest(highest_promised);
	preparer.n_highest_received = -1;
	preparer.numOpen = 0;
	
	msg.PrepareRequest(paxosID, preparer.n);
	
	SendMsgToAll();
	
	scheduler->Reset(&prepareTimeout);
}

// returns whether to put out an IORead
bool PaxosInstance::TryFinishPrepare()
{
	Log_Trace();
	
	/*	Lamport: If the proposer receives a response to its prepare requests
		(numbered n) from a majority of acceptors, then it sends an accept (propose)
		request to each of those acceptors for a proposal numbered n with a 
		value v, where v is the value of the highest-numbered proposal among 
		the responses, or is any value if the responses reported no proposals.
	*/
		
	if (preparer.n < highest_promised || numRejected >= ceil(config.numNodes / 2))
	{
		Prepare();
		return false;
	}
	
	// see if we have enough positive replies to advance	
	int numPositive = numReceived - numRejected;
	if (numPositive >= config.MinMajority())
	{
		if (preparer.numOpen == numPositive)
		{
			// free to propose my value, stored in proposer, set by the application
			Propose();
			return false;
		}
		else
		{
			// use the highest numbered proposal returned to me, this is stored in the preparer
			proposer.value.Set(preparer.value); // todo: retval
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

void PaxosInstance::Propose()
{
	Log_Trace();
	
	StopPreparer();

	proposer.active = true;
	proposer.n = preparer.n;

	// proposer.length and proposer.value are filled out by FinishPrepare()
	msg.ProposeRequest(paxosID, preparer.n, proposer.value);
	SendMsgToAll();
	
	scheduler->Reset(&proposeTimeout);
}

// returns whether to put out an IORead
bool PaxosInstance::TryFinishPropose()
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

void PaxosInstance::OnPrepareTimeout()
{
	Log_Trace();
	
	assert(preparer.active);
	
	Prepare();
}

void PaxosInstance::OnProposeTimeout()
{
	Log_Trace();
	
	assert(proposer.active);
	
	Prepare();
}
