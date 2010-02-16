#include "ReplicatedLog.h"
#include <string.h>
#include "System/Events/EventLoop.h"
#include "Framework/Transport/TransportTCPReader.h"
#include "Framework/Transport/TransportTCPWriter.h"
#include "Framework/Transport/TransportUDPReader.h"
#include "Framework/Transport/TransportUDPWriter.h"
#include <stdlib.h>

ReplicatedLog* replicatedLog = NULL;

ReplicatedLog* ReplicatedLog::Get()
{
	if (replicatedLog == NULL)
		replicatedLog = new ReplicatedLog;
	
	return replicatedLog;
}

ReplicatedLog::ReplicatedLog()
:	onRead(this, &ReplicatedLog::OnRead),
	onLearnLease(this, &ReplicatedLog::OnLearnLease),
	onLeaseTimeout(this, &ReplicatedLog::OnLeaseTimeout)
{
	reader = NULL;
	writers = NULL;
}

bool ReplicatedLog::Init(bool useSoftClock)
{
	Log_Trace();
	
	replicatedDB = NULL;
	
	InitTransport();

	proposer.Init(writers);
	acceptor.Init(writers);
	learner.Init(writers);
	
	highestPaxosID = 0;
	lastStarted = EventLoop::Now();
	lastLength = 0;
	lastTook = 0;
	thruput = 0;
	
	proposer.paxosID = acceptor.paxosID;
	learner.paxosID = acceptor.paxosID;
	
	masterLease.Init(useSoftClock);
	masterLease.SetOnLearnLease(&onLearnLease);
	masterLease.SetOnLeaseTimeout(&onLeaseTimeout);
	masterLease.AcquireLease();
	
	logCache.Init();
	
	safeDB = false;
	
	return true;
}

void ReplicatedLog::InitTransport()
{
	unsigned	i;
	Endpoint	endpoint;

	reader = new TransportTCPReader;

	if (!reader->Init(RCONF->GetPort()))
		STOP_FAIL("cannot bind Paxos port", 1);
	reader->SetOnRead(&onRead);

	writers =
			(TransportTCPWriter**)
			Alloc(sizeof(TransportTCPWriter*) * RCONF->GetNumNodes());
	
	for (i = 0; i < RCONF->GetNumNodes(); i++)
	{
		endpoint = RCONF->GetEndpoint(i);
		endpoint.SetPort(endpoint.GetPort());
		writers[i] = new TransportTCPWriter;
		Log_Trace("Connecting to %s", endpoint.ToString());
		if (!writers[i]->Init(endpoint))
			STOP_FAIL("cannot bind Paxos port", 1);
	}
}

void ReplicatedLog::Shutdown()
{
	masterLease.Shutdown();
	acceptor.Shutdown();

	delete reader;
	if (writers)
	{
		for (unsigned i = 0; i < RCONF->GetNumNodes(); i++)
			delete writers[i];
		
		free(writers);
	}

	delete this;
}

void ReplicatedLog::StopPaxos()
{
	Log_Trace();
	
	reader->Stop();
}

void ReplicatedLog::StopMasterLease()
{
	masterLease.Stop();
}

void ReplicatedLog::ContinuePaxos()
{
	Log_Trace();
	
	reader->Continue();
}

void ReplicatedLog::ContinueMasterLease()
{
	masterLease.Continue();
}

bool ReplicatedLog::IsPaxosActive()
{
	return reader->IsActive();
}

bool ReplicatedLog::IsMasterLeaseActive()
{
	return masterLease.IsActive();
}


bool ReplicatedLog::Append(ByteString &value_)
{
	Log_Trace();
	
	if (!logQueue.Push(value_))
	{
		ASSERT_FAIL();
		return false;
	}
	
	if (!proposer.IsActive())
	{
		if (!rmsg.Init(RCONF->GetNodeID(),
					   RCONF->GetRestartCounter(),
					   masterLease.GetLeaseEpoch(), value_))
		{	
			ASSERT_FAIL();
			return false;
		}
		
		if (!rmsg.Write(value))
		{
			ASSERT_FAIL();
			return false;
		}
		
		return proposer.Propose(value);
	}
	else
		ASSERT_FAIL();
	
	return true;
}

void ReplicatedLog::SetReplicatedDB(ReplicatedDB* replicatedDB_)
{
	replicatedDB = replicatedDB_;
}

uint64_t ReplicatedLog::GetPaxosID()
{
	return proposer.paxosID;
}

void ReplicatedLog::SetPaxosID(Transaction* transaction, uint64_t paxosID)
{
	EventLoop::Remove(&(proposer.prepareTimeout));
	EventLoop::Remove(&(proposer.proposeTimeout));
	proposer.paxosID = paxosID;
	proposer.state.Init();

	acceptor.paxosID = paxosID;
	acceptor.state.Init();
	acceptor.Persist(transaction);

	learner.paxosID = paxosID;
	learner.state.Init();
}

bool ReplicatedLog::IsMaster()
{
	return masterLease.IsLeaseOwner();
}

int ReplicatedLog::GetMaster()
{
	return masterLease.GetLeaseOwner();
}

unsigned ReplicatedLog::GetNumNodes()
{
	return RCONF->GetNumNodes();
}

unsigned ReplicatedLog::GetNodeID()
{
	return RCONF->GetNodeID();
}

void ReplicatedLog::OnRead()
{
	Log_Trace();
	
	ByteString bs;
	reader->GetMessage(bs);
	pmsg.Read(bs);
	
	Log_Trace("Received paxos msg: %.*s", bs.length, bs.buffer);
	
	ProcessMsg();
}

void ReplicatedLog::ProcessMsg()
{
	if (pmsg.paxosID > highestPaxosID)
		highestPaxosID = pmsg.paxosID;
	
	if (pmsg.type == PAXOS_PREPARE_REQUEST)
		OnPrepareRequest();
	else if (pmsg.IsPrepareResponse())
		OnPrepareResponse();
	else if (pmsg.type == PAXOS_PROPOSE_REQUEST)
		OnProposeRequest();
	else if (pmsg.IsProposeResponse())
		OnProposeResponse();
	else if (pmsg.IsLearn())
		OnLearnChosen();
	else if (pmsg.type == PAXOS_REQUEST_CHOSEN)
		OnRequestChosen();
	else if (pmsg.type = PAXOS_START_CATCHUP)
		OnStartCatchup();
	else
		ASSERT_FAIL();
}

void ReplicatedLog::OnPrepareRequest()
{
	Log_Trace();
		
	if (pmsg.paxosID == acceptor.paxosID)
		return acceptor.OnPrepareRequest(pmsg);

	OnRequest();
}

void ReplicatedLog::OnPrepareResponse()
{
	Log_Trace();
	
	if (pmsg.paxosID == proposer.paxosID)
		proposer.OnPrepareResponse(pmsg);
}

void ReplicatedLog::OnProposeRequest()
{
	Log_Trace();
	
	if (pmsg.paxosID == acceptor.paxosID)
		return acceptor.OnProposeRequest(pmsg);
	
	OnRequest();
}

void ReplicatedLog::OnProposeResponse()
{
	Log_Trace();

	if (pmsg.paxosID == proposer.paxosID)
		proposer.OnProposeResponse(pmsg);
}

void ReplicatedLog::OnLearnChosen()
{
	Log_Trace();

	uint64_t paxosID;
	bool	ownAppend, clientAppend, commit;

	if (pmsg.paxosID == learner.paxosID)
	{
		if (pmsg.type == PAXOS_LEARN_PROPOSAL && acceptor.state.accepted &&
			acceptor.state.acceptedProposalID == pmsg.proposalID)
				pmsg.LearnValue(pmsg.paxosID,
				GetNodeID(),
				acceptor.state.acceptedValue);
		
		if (pmsg.type == PAXOS_LEARN_VALUE)
			learner.OnLearnChosen(pmsg);
		else
		{
			learner.RequestChosen(pmsg.nodeID);
			return;
		}
		
		// save it in the logCache (includes the epoch info)
		commit = learner.paxosID == (highestPaxosID - 1);
		logCache.Push(learner.paxosID, learner.state.value, commit);
		
		rmsg.Read(learner.state.value);	// rmsg.value holds the 'user value'
		paxosID = learner.paxosID;		// this is the value we
										// pass to the ReplicatedDB
		
		NewPaxosRound();
		// increments paxosID, clears proposer, acceptor, learner
		
		if (highestPaxosID > paxosID)
		{
			learner.RequestChosen(pmsg.nodeID);
		}

		Log_Trace("%d %d %" PRIu64 " %" PRIu64 "",
			rmsg.nodeID, GetNodeID(),
			rmsg.restartCounter, RCONF->GetRestartCounter());
		
		if (rmsg.nodeID == GetNodeID() &&
			rmsg.restartCounter == RCONF->GetRestartCounter())
		{
			logQueue.Pop(); // we just appended this
		}

		if (rmsg.nodeID == GetNodeID() &&
			rmsg.restartCounter == RCONF->GetRestartCounter() &&
			rmsg.leaseEpoch == masterLease.GetLeaseEpoch() &&
			masterLease.IsLeaseOwner())
		{
			proposer.state.leader = true;
			Log_Trace("Multi paxos enabled");
		}
		else
		{
			proposer.state.leader = false;
			Log_Trace("Multi paxos disabled");
		}
		
		if (rmsg.nodeID == GetNodeID() &&
		rmsg.restartCounter == RCONF->GetRestartCounter())
		{
			ownAppend = true;
		}
		else
		{
			ownAppend = false;
		}
		
		// commit chaining
		if (ownAppend && rmsg.value == BS_MSG_NOP)
		{
			safeDB = true;
			if (replicatedDB != NULL && IsMaster())
				replicatedDB->OnMasterLease(masterLease.IsLeaseOwner());
		}
		else if (replicatedDB != NULL &&
				 rmsg.value.length > 0 &&
				 !(rmsg.value == BS_MSG_NOP))
		{
			if (!GetTransaction()->IsActive())
			{
				Log_Trace("starting new transaction");
				GetTransaction()->Begin();
			}
			clientAppend = ownAppend &&
						   rmsg.leaseEpoch == masterLease.GetLeaseEpoch() &&
						   IsMaster();
			
			replicatedDB->OnAppend(GetTransaction(),
				paxosID,
				rmsg.value,
				clientAppend);
			// client will stop Paxos here
		}
		
		if (!proposer.IsActive() && logQueue.Length() > 0)
		{
			if (!rmsg.Init(RCONF->GetNodeID(), RCONF->GetRestartCounter(),
						   masterLease.GetLeaseEpoch(), *(logQueue.Next())))
				ASSERT_FAIL();
			
			if (!rmsg.Write(value))
				ASSERT_FAIL();
			
			proposer.Propose(value);
		}
	}
	else if (pmsg.paxosID > learner.paxosID)
	{
		//	I am lagging and need to catch-up
		
		learner.RequestChosen(pmsg.nodeID);
	}
}

void ReplicatedLog::OnRequestChosen()
{
	Log_Trace();

	ByteString value_;
	
	if (pmsg.paxosID == learner.paxosID)
		return learner.OnRequestChosen(pmsg);
	
	// same as OnRequest but with the learner's message
	if (pmsg.paxosID < learner.paxosID)
	{
		// the node is lagging and needs to catch-up
		if (logCache.Get(pmsg.paxosID, value_))
		{
			Log_Trace("Sending paxosID %d to node %d", pmsg.paxosID, pmsg.nodeID);
			learner.SendChosen(pmsg.nodeID, pmsg.paxosID, value_);
		}
		else
		{
			Log_Trace("Node requested a paxosID I no longer have");
			learner.SendStartCatchup(pmsg.nodeID, pmsg.paxosID);
		}
	}
}

void ReplicatedLog::OnStartCatchup()
{
	Log_Trace();

	if (pmsg.paxosID == learner.paxosID)
	{
		if (replicatedDB != NULL && !replicatedDB->IsCatchingUp() && masterLease.IsLeaseKnown())
			replicatedDB->OnDoCatchup(masterLease.GetLeaseOwner());
	}
}

void ReplicatedLog::OnRequest()
{
	Log_Trace();
	
	ByteString value_;

	if (pmsg.paxosID < acceptor.paxosID)
	{
		// the node is lagging and needs to catch-up
		if (logCache.Get(pmsg.paxosID, value_))
			learner.SendChosen(pmsg.nodeID, pmsg.paxosID, value_);
	}
	else // paxosID < msg.paxosID
	{
		//	I am lagging and need to catch-up
		
		learner.RequestChosen(pmsg.nodeID);
	}
}

void ReplicatedLog::NewPaxosRound()
{
	uint64_t now;

	now = EventLoop::Now();
	lastTook = ABS(now - lastStarted);
	lastLength = learner.state.value.length;
	thruput = (uint64_t)(lastLength / (lastTook / 1000.0));
	lastStarted = now;
	
	EventLoop::Remove(&(proposer.prepareTimeout));
	EventLoop::Remove(&(proposer.proposeTimeout));
	proposer.paxosID++;
	proposer.state.Init();

	acceptor.paxosID++;
	acceptor.state.Init();

	learner.paxosID++;
	learner.state.Init();
	
	masterLease.OnNewPaxosRound();
}

void ReplicatedLog::OnLearnLease()
{
	if (masterLease.IsLeaseOwner() &&
		!safeDB &&
		!(proposer.IsActive() &&
		rmsg.value == BS_MSG_NOP))
	{
		ByteString	nop = BS_MSG_NOP;
		
		Log_Trace("appending NOP to assure safeDB");
		Append(nop);
	}
}

void ReplicatedLog::OnLeaseTimeout()
{
	safeDB = false;
	
	if (replicatedDB)
	{
		proposer.Stop();
		replicatedDB->OnMasterLeaseExpired();
	}
}

bool ReplicatedLog::IsAppending()
{
	return IsMaster() && proposer.state.numProposals > 0;
}

Transaction* ReplicatedLog::GetTransaction()
{
	return &acceptor.transaction;
}

bool ReplicatedLog::IsSafeDB()
{
	return safeDB;
}

void ReplicatedLog::OnPaxosLeaseMsg(uint64_t paxosID, unsigned nodeID)
{
	if (paxosID > learner.paxosID)
	{
		if (paxosID > highestPaxosID)
			highestPaxosID = paxosID;

		// I am lagging and need to catch-up
		
		if (IsPaxosActive())
			learner.RequestChosen(nodeID);
	}
}

uint64_t ReplicatedLog::GetLastRound_Length()
{
	return lastLength;
}

uint64_t ReplicatedLog::GetLastRound_Time()
{
	return lastTook;
}

uint64_t ReplicatedLog::GetLastRound_Thruput()
{
	return thruput;
}

