#include "ReplicatedLog.h"
#include <string.h>
#include "System/Events/EventLoop.h"
#include "Framework/Paxos/PaxosConsts.h"
#include "Framework/Transport/TransportTCPReader.h"
#include "Framework/Transport/TransportTCPWriter.h"
#include "Framework/Transport/TransportUDPReader.h"
#include "Framework/Transport/TransportUDPWriter.h"
#include <stdlib.h>

ReplicatedLog replicatedLog;

ReplicatedLog* ReplicatedLog::Get()
{
	return &replicatedLog;
}

ReplicatedLog::ReplicatedLog()
:	onRead(this, &ReplicatedLog::OnRead),
	onCatchupTimeout(this, &ReplicatedLog::OnCatchupTimeout),
	catchupTimeout(CATCHUP_TIMEOUT, &onCatchupTimeout),
	onLearnLease(this, &ReplicatedLog::OnLearnLease),
	onLeaseTimeout(this, &ReplicatedLog::OnLeaseTimeout)
{
}

bool ReplicatedLog::Init()
{
	Log_Trace();
	
	replicatedDB = NULL;
	
	InitTransport();

	proposer.Init(writers);
	acceptor.Init(writers);
	learner.Init(writers);
	
	proposer.paxosID = acceptor.paxosID;
	learner.paxosID = acceptor.paxosID;
	
	masterLease.Init();
	masterLease.SetOnLearnLease(&onLearnLease);
	masterLease.SetOnLeaseTimeout(&onLeaseTimeout);
	//if (PaxosConfig::Get()->nodeID == 0) // TODO: FOR DEBUGGING KGJDFKGJDFKGJDFKLGJDLKFJGLDKFJGDLKFJGDLFKGJ
		masterLease.AcquireLease();
	
	appending = false;
	safeDB = false;
	
	return true;
}

void ReplicatedLog::InitTransport()
{
	unsigned	i;
	Endpoint	endpoint;

#if USE_TCP == 1
	reader = new TransportTCPReader;
#else
	reader = new TransportUDPReader;
#endif
	if (!reader->Init(PaxosConfig::Get()->port + PAXOS_PORT_OFFSET))
		ASSERT_FAIL();
	reader->SetOnRead(&onRead);

	writers = (TransportWriter**) malloc(sizeof(TransportWriter*) * PaxosConfig::Get()->numNodes);
	for (i = 0; i < PaxosConfig::Get()->numNodes; i++)
	{
		endpoint = PaxosConfig::Get()->endpoints[i];
		endpoint.SetPort(endpoint.GetPort() + PAXOS_PORT_OFFSET);
#if USE_TCP == 1
		writers[i] = new TransportTCPWriter;
#else
		writers[i] = new TransportUDPWriter;
#endif
		Log_Message("Connecting to %s", endpoint.ToString());
		if (!writers[i]->Init(endpoint))
			ASSERT_FAIL();
	}
}

void ReplicatedLog::Stop()
{
	reader->Stop();
	masterLease.Stop();
}

void ReplicatedLog::Continue()
{
	reader->Continue();
	masterLease.Continue();
}

bool ReplicatedLog::Append(ByteString value_)
{
	Log_Trace();
	
	if (!logQueue.Push(value_))
		return false;
	
	if (!appending)
	{
		if (!rmsg.Init(PaxosConfig::Get()->nodeID, PaxosConfig::Get()->restartCounter,
					   masterLease.GetLeaseEpoch(), value_))
			return false;
		
		if (!rmsg.Write(value))
			return false;
		
		appending = true;
		return proposer.Propose(value);
	}
	
	return true;
}

void ReplicatedLog::SetReplicatedDB(ReplicatedDB* replicatedDB_)
{
	replicatedDB = replicatedDB_;
}

bool ReplicatedLog::GetLogItem(uint64_t paxosID, ByteString& value)
{
	return logCache.Get(paxosID, value);
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

unsigned ReplicatedLog::GetNodeID()
{
	return PaxosConfig::Get()->nodeID;
}

void ReplicatedLog::OnRead()
{
	Log_Trace();
	
	ByteString bs;
	reader->GetMessage(bs);
	pmsg.Read(bs);
	
	ProcessMsg();
}

void ReplicatedLog::ProcessMsg()
{
	if (pmsg.paxosID > highestPaxosID)
		highestPaxosID = pmsg.paxosID;
	
	if (pmsg.type == PREPARE_REQUEST)
		OnPrepareRequest();
	else if (pmsg.type == PREPARE_RESPONSE)
		OnPrepareResponse();
	else if (pmsg.type == PROPOSE_REQUEST)
		OnProposeRequest();
	else if (pmsg.type == PROPOSE_RESPONSE)
		OnProposeResponse();
	else if (pmsg.type == LEARN_CHOSEN)
		OnLearnChosen();
	else if (pmsg.type == REQUEST_CHOSEN)
		OnRequestChosen();
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
	bool	ownAppend;

	if (pmsg.paxosID == learner.paxosID)
	{
		if (catchupTimeout.IsActive())
			EventLoop::Remove(&catchupTimeout);
		
		if (pmsg.subtype == LEARN_PROPOSAL && acceptor.state.accepted &&
			acceptor.state.acceptedProposalID == pmsg.proposalID)
				pmsg.LearnChosen(pmsg.paxosID, GetNodeID(), LEARN_VALUE, acceptor.state.acceptedValue);
		
		if (pmsg.subtype == LEARN_VALUE)
			learner.OnLearnChosen(pmsg);
		else
		{
			learner.RequestChosen(pmsg.nodeID);
			return;
		}
		
		// save it in the logCache (includes the epoch info)
		logCache.Push(learner.paxosID, learner.state.value);
		
		rmsg.Read(learner.state.value);	// rmsg.value holds the 'user value'
		paxosID = learner.paxosID;		// this is the value we pass to the ReplicatedDB
		
		NewPaxosRound(); // increments paxosID, clears proposer, acceptor, learner
		
		if (highestPaxosID > paxosID)
		{
			learner.RequestChosen(pmsg.nodeID);
			return;
		}

		Log_Message("%d %d %" PRIu64 " %" PRIu64 "", rmsg.nodeID, GetNodeID(), rmsg.restartCounter,
			PaxosConfig::Get()->restartCounter);
		if (rmsg.nodeID == GetNodeID() && rmsg.restartCounter == PaxosConfig::Get()->restartCounter)
			logQueue.Pop(); // we just appended this

		if (rmsg.nodeID == GetNodeID() && rmsg.restartCounter == PaxosConfig::Get()->restartCounter &&
			rmsg.leaseEpoch == masterLease.GetLeaseEpoch() && masterLease.IsLeaseOwner())
		{
			proposer.state.leader = true;
			Log_Message("Multi paxos enabled");
		}
		else
		{
			proposer.state.leader = false;
			Log_Message("Multi paxos disabled");
		}

		
		if (logQueue.Length() > 0)
		{
			if (!rmsg.Init(GetNodeID(), PaxosConfig::Get()->restartCounter, 
				masterLease.GetLeaseEpoch(), *(logQueue.Next())))
					ASSERT_FAIL();
		
			if (!rmsg.Write(value))
				ASSERT_FAIL();
			
			appending = true;
			proposer.Propose(value);
		}
		else
			appending = false;
		
		if (rmsg.nodeID == GetNodeID() && rmsg.restartCounter == PaxosConfig::Get()->restartCounter)
			ownAppend = true;
		else
			ownAppend = false;
		
		// commit chaining
		if (ownAppend && rmsg.value == BS_MSG_NOP)
		{
			safeDB = true;
			if (replicatedDB != NULL & IsMaster())
				replicatedDB->OnMasterLease(masterLease.IsLeaseOwner());
		}
		else if (replicatedDB != NULL && rmsg.value.length > 0)
			{
				if (!acceptor.transaction.IsActive())
					acceptor.transaction.Begin();
				replicatedDB->OnAppend(&acceptor.transaction, paxosID, rmsg.value, ownAppend);
				// client calls Append() here
			}
	}
	else if (pmsg.paxosID > learner.paxosID)
	{
		//	I am lagging and need to catch-up
		
		if (!catchupTimeout.IsActive())
			EventLoop::Add(&catchupTimeout);
		
		learner.RequestChosen(pmsg.nodeID);
	}
}

void ReplicatedLog::OnRequestChosen()
{
	ByteString value;
	
	if (pmsg.paxosID == learner.paxosID)
		return learner.OnRequestChosen(pmsg);
	
	// same as OnRequest but with the learner's message
	if (pmsg.paxosID < learner.paxosID)
	{
		// the node is lagging and needs to catch-up
		if (logCache.Get(pmsg.paxosID, value))
			learner.SendChosen(pmsg.nodeID, pmsg.paxosID, value);
	}
}

void ReplicatedLog::OnRequest()
{
	Log_Trace();
	
	ByteString value;

	if (pmsg.paxosID < acceptor.paxosID)
	{
		// the node is lagging and needs to catch-up
		if (logCache.Get(pmsg.paxosID, value))
			learner.SendChosen(pmsg.nodeID, pmsg.paxosID, value);
	}
	else // paxosID < msg.paxosID
	{
		//	I am lagging and need to catch-up
		
		if (!catchupTimeout.IsActive())
			EventLoop::Add(&catchupTimeout);
		
		learner.RequestChosen(pmsg.nodeID);
	}
}

void ReplicatedLog::NewPaxosRound()
{
	EventLoop::Remove(&(proposer.prepareTimeout));
	EventLoop::Remove(&(proposer.proposeTimeout));
	proposer.paxosID++;
	proposer.state.Init();

	acceptor.paxosID++;
	acceptor.state.Init();

	learner.paxosID++;
	learner.state.Init();
}

void ReplicatedLog::OnCatchupTimeout()
{
	Log_Trace();

	if (replicatedDB != NULL && masterLease.IsLeaseKnown())
		replicatedDB->OnDoCatchup(masterLease.GetLeaseOwner());
}

void ReplicatedLog::OnLearnLease()
{
	if (masterLease.IsLeaseOwner() && !safeDB && !(appending && rmsg.value == BS_MSG_NOP))
	{
		Log_Message("appending NOP");
		Append(BS_MSG_NOP);
	}
}

void ReplicatedLog::OnLeaseTimeout()
{
	safeDB = false;
	
	if (replicatedDB)
		replicatedDB->OnMasterLeaseExpired();
}

bool ReplicatedLog::IsAppending()
{
	return appending;
}

Transaction* ReplicatedLog::GetTransaction()
{
	return &acceptor.transaction;
}

bool ReplicatedLog::IsSafeDB()
{
	return safeDB;
}
