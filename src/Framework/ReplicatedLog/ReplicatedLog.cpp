#include "ReplicatedLog.h"
#include <string.h>
#include "Framework/Paxos/PaxosConsts.h"
#include "Framework/Transport/TransportTCPReader.h"
#include "Framework/Transport/TransportTCPWriter.h"
#include <stdlib.h>

ReplicatedLog::ReplicatedLog()
:	onRead(this, &ReplicatedLog::OnRead),
	onCatchupTimeout(this, &ReplicatedLog::OnCatchupTimeout),
	catchupTimeout(CATCHUP_TIMEOUT, &onCatchupTimeout),
	onLearnLease(this, &ReplicatedLog::OnLearnLease),
	onLeaseTimeout(this, &ReplicatedLog::OnLeaseTimeout)
{
}

bool ReplicatedLog::Init(IOProcessor* ioproc_, Scheduler* scheduler_, char* filename)
{
	// I/O framework
	scheduler = scheduler_;
	
	replicatedDB = NULL;
	
	if (!config.Init(filename))
		return false;

	masterLease.Init(ioproc_, scheduler_, this, &config);
	masterLease.SetOnLearnLease(&onLearnLease);
	masterLease.SetOnLeaseTimeout(&onLeaseTimeout);
	masterLease.AcquireLease();

	InitTransport(ioproc_, scheduler_, &config);

	proposer.Init(writers, scheduler_, &config);
	acceptor.Init(writers, scheduler_, &config);
	learner.Init(writers, scheduler_, &config);
	
	appending = false;
	safeDB = false;
	
	return true;
}

void ReplicatedLog::InitTransport(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_)
{
	int			i;
	Endpoint	endpoint;
	Endpoint*	it;

	reader = new TransportTCPReader;
	reader->Init(ioproc_, config_->port + PAXOS_PORT_OFFSET);
	reader->SetOnRead(&onRead);
	
	writers = (TransportWriter**) malloc(sizeof(TransportWriter*) * config_->numNodes);
	it = config_->endpoints.Head();
	for (i = 0; i < config_->numNodes; i++)
	{
		endpoint = *it;
		endpoint.SetPort(endpoint.GetPort() + PAXOS_PORT_OFFSET);
		writers[i] = new TransportTCPWriter;
		
		Log_Message("Connecting to %s", endpoint.ToString());
		writers[i]->Init(ioproc_, scheduler_, endpoint);
	
		it = config_->endpoints.Next(it);
	}
}

/*bool ReplicatedLog::Stop()
{
	sendtoall = false;
	
	if (udpread.active)
		ioproc->Remove(&udpread);

	if (udpwrite.active)
		ioproc->Remove(&udpwrite);
	
	for (int i = 0; i < SIZE(replicatedDBs); i++)
		if (replicatedDBs[i] != NULL)
			replicatedDBs[i]->OnStop();
	
	return true;
}*/

/*bool ReplicatedLog::Continue()
{
	ioproc->Add(&udpwrite);
	
	for (int i = 0; i < SIZE(replicatedDBs); i++)
		if (replicatedDBs[i] != NULL)
			replicatedDBs[i]->OnContinue();

	return true;
}*/

bool ReplicatedLog::Append(ByteString value_)
{
	if (!logQueue.Push(value_))
		return false;
	
	if (!appending)
	{
		if (!rmsg.Init(config.nodeID, config.restartCounter,  masterLease.GetLeaseEpoch(), value_))
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

bool ReplicatedLog::GetLogItem(ulong64 paxosID, ByteString& value)
{
	return logCache.Get(paxosID, value);
}

ulong64 ReplicatedLog::GetPaxosID()
{
	return proposer.paxosID;
}

bool ReplicatedLog::IsMaster()
{
	return masterLease.IsLeaseOwner();
}

int ReplicatedLog::GetNodeID()
{
	return config.nodeID;
}

void ReplicatedLog::OnRead()
{
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

	ulong64 paxosID;
	bool	ownAppend;

	if (pmsg.paxosID == learner.paxosID)
	{
		if (catchupTimeout.IsActive())
			scheduler->Remove(&catchupTimeout);
		
		if (pmsg.subtype == LEARN_PROPOSAL && acceptor.state.accepted &&
			acceptor.state.acceptedProposalID == pmsg.proposalID)
				pmsg.LearnChosen(pmsg.paxosID, config.nodeID, LEARN_VALUE, acceptor.state.acceptedValue);
		
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

		if (rmsg.nodeID == config.nodeID && rmsg.restartCounter == config.restartCounter)
			logQueue.Pop(); // we just appended this

		if (rmsg.nodeID == config.nodeID && rmsg.restartCounter == config.restartCounter &&
			rmsg.leaseEpoch == masterLease.GetLeaseEpoch() && masterLease.IsLeaseOwner())
		{
			proposer.state.leader = true;
			Log_Message("Multi paxos enabled!");
		}
		else
		{
			proposer.state.leader = false;
		}

		
		if (logQueue.Size() > 0)
		{
			if (!rmsg.Init(config.nodeID, config.restartCounter, 
				masterLease.GetLeaseEpoch(), *(logQueue.Next())))
					ASSERT_FAIL();
		
			if (!rmsg.Write(value))
				ASSERT_FAIL();
			
			appending = true;
			proposer.Propose(value);
		}
		else
			appending = false;
		
		if (rmsg.nodeID == config.nodeID && rmsg.restartCounter == config.restartCounter)
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
			scheduler->Add(&catchupTimeout);
		
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
			scheduler->Add(&catchupTimeout);
		
		learner.RequestChosen(pmsg.nodeID);
	}
}

void ReplicatedLog::NewPaxosRound()
{
	proposer.scheduler->Remove(&(proposer.prepareTimeout));
	proposer.scheduler->Remove(&(proposer.proposeTimeout));
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

	if (replicatedDB != NULL)
		replicatedDB->OnDoCatchup();
}

void ReplicatedLog::OnLearnLease()
{
	if (masterLease.IsLeaseOwner() && !safeDB)
		Append(BS_MSG_NOP);
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
