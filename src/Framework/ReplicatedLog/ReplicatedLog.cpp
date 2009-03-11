#include "ReplicatedLog.h"
#include <string.h>
#include "PaxosConsts.h"

ReplicatedLog::ReplicatedLog()
:	onCatchupTimeout(this, &ReplicatedLog::OnCatchupTimeout),
	catchupTimeout(CATCHUP_TIMEOUT, &onCatchupTimeout),
	onLearnLease(this, &ReplicatedLog::OnLearnLease),
	onLeaseTimeout(this, &ReplicatedLog::OnLeaseTimeout)
{
}

bool ReplicatedLog::Init(IOProcessor* ioproc_, Scheduler* scheduler_, char* filename)
{
	// I/O framework
	ioproc = ioproc_;
	scheduler = scheduler_;
	
	if (!config.Init(filename))
		return false;

	PaxosProposer::Init(ioproc_, scheduler_, &config);
	PaxosAcceptor::Init(ioproc_, scheduler_, &config);
	PaxosLearner::Init(ioproc_, scheduler_, &config);
	
	masterLease.Init(ioproc_, scheduler_, &config);
	masterLease.SetOnLearnLease(&onLearnLease);
	masterLease.SetOnLeaseTimeout(&onLeaseTimeout);
	masterLease.AcquireLease();
	
	appending = false;
	
	return true;
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
	if (!logQueue.Push(value_.buffer[0], value_))
		return false;
	
	if (!appending)
	{
		if (!ReplicatedLog::msg.Init(config.nodeID, config.restartCounter, 
			masterLease.LeaseEpoch(), value_))
				return false;
		
		if (!ReplicatedLog::msg.Write(value))
			return false;
		
		appending = true;
		return PaxosProposer::Propose(value);
	}
	
	return true;
}

void ReplicatedLog::SetReplicatedDB(ReplicatedDB* replicatedDB_)
{
	replicatedDB = replicatedDB_;
}

LogItem* ReplicatedLog::LastLogItem()
{
	return logCache.Last();
}

bool ReplicatedLog::IsMaster()
{
	return masterLease.IsLeaseOwner();
}

int ReplicatedLog::NodeID()
{
	return config.nodeID;
}

void ReplicatedLog::OnPrepareRequest()
{
	Log_Trace();
		
	if (PaxosAcceptor::msg.paxosID == PaxosAcceptor::paxosID)
		return PaxosAcceptor::OnPrepareRequest();

	if (PaxosAcceptor::msg.paxosID > highestPaxosID)
		highestPaxosID = PaxosAcceptor::msg.paxosID;
	
	OnRequest();
	
	PaxosAcceptor::ioproc->Add(&(PaxosAcceptor::udpread));
}

void ReplicatedLog::OnPrepareResponse()
{
	Log_Trace();
	
	if (PaxosProposer::msg.paxosID == PaxosProposer::paxosID)
		PaxosProposer::OnPrepareResponse();
	else
		PaxosProposer::ioproc->Add(&(PaxosProposer::udpread));
}

void ReplicatedLog::OnProposeRequest()
{
	Log_Trace();
	
	if (PaxosAcceptor::msg.paxosID == PaxosAcceptor::paxosID)
		return PaxosAcceptor::OnProposeRequest();
	
	if (PaxosAcceptor::msg.paxosID > highestPaxosID)
		highestPaxosID = PaxosAcceptor::msg.paxosID;

	OnRequest();
	
	PaxosAcceptor::ioproc->Add(&(PaxosAcceptor::udpread));
}

void ReplicatedLog::OnProposeResponse()
{
		Log_Trace();
	
	if (PaxosProposer::msg.paxosID == PaxosProposer::paxosID)
		PaxosProposer::OnProposeResponse();
	else
		PaxosProposer::ioproc->Add(&(PaxosProposer::udpread));
}

void ReplicatedLog::OnLearnChosen()
{
	Log_Trace();

	ulong64 paxosID;
	
	if (PaxosLearner::msg.paxosID > highestPaxosID)
		highestPaxosID = PaxosLearner::msg.paxosID;

	if (PaxosLearner::msg.paxosID == PaxosLearner::paxosID)
	{
		if (catchupTimeout.IsActive())
			scheduler->Remove(&catchupTimeout);
		
		PaxosLearner::OnLearnChosen();
		// save it in the logCache (includes the epoch info)
		logCache.Push(PaxosLearner::paxosID, PaxosLearner::state.value);
		
		msg.Read(PaxosLearner::state.value);	// msg.value holds the 'user value'
		paxosID = PaxosLearner::paxosID;		// this is the value we pass to the ReplicatedDB
		
		NewPaxosRound(); // increments paxosID, clears proposer, acceptor, learner
		
		if (highestPaxosID > paxosID)
		{
			PaxosLearner::RequestChosen(PaxosLearner::udpread.endpoint);
			return;
		}

		if (msg.nodeID == config.nodeID && msg.restartCounter == config.restartCounter)
			logQueue.Pop(); // we just appended this

		if (msg.nodeID == config.nodeID && msg.restartCounter == config.restartCounter &&
			msg.leaseEpoch == masterLease.LeaseEpoch() && masterLease.IsLeaseOwner())
		{
			PaxosProposer::state.leader = true;
			Log_Message("Multi paxos enabled!");
		}
		else
		{
			PaxosProposer::state.leader = false;
		}

		
		if (logQueue.Size() > 0)
		{
			if (!ReplicatedLog::msg.Init(config.nodeID, config.restartCounter, 
				masterLease.LeaseEpoch(), logQueue.Next()->value))
					ASSERT_FAIL();
		
			if (!ReplicatedLog::msg.Write(value))
				ASSERT_FAIL();
			
			appending = true;
			PaxosProposer::Propose(value);
		}
		else
			appending = false;
		
		if (msg.nodeID == config.nodeID && msg.restartCounter == config.restartCounter &&
			replicatedDB != NULL && msg.value.length > 0)
		{
			//Transaction tx(table);
			replicatedDB->OnAppend(NULL, paxosID, msg.value); // user will call Append() here
			//tx.Commit();
		}		
	}
	else if (PaxosLearner::msg.paxosID > PaxosLearner::paxosID)
	{
		//	I am lagging and need to catch-up
		
		if (PaxosLearner::msg.paxosID > highestPaxosID)
			highestPaxosID = PaxosLearner::msg.paxosID;
		
		if (!catchupTimeout.IsActive())
			scheduler->Add(&catchupTimeout);
		
			Endpoint endpoint = PaxosLearner::udpread.endpoint;
			endpoint.SetPort(endpoint.GetPort() - PAXOS_PROPOSER_PORT_OFFSET
												+ PAXOS_LEARNER_PORT_OFFSET);
			PaxosLearner::RequestChosen(endpoint);
	}
}

void ReplicatedLog::OnRequestChosen()
{
	if (PaxosLearner::msg.paxosID == PaxosLearner::paxosID)
		return PaxosLearner::OnRequestChosen();
	
	if (PaxosLearner::msg.paxosID > highestPaxosID)
		highestPaxosID = PaxosLearner::msg.paxosID;

	// same as OnRequest but with the learner's message
	if (PaxosLearner::msg.paxosID < PaxosLearner::paxosID)
	{
		// the node is lagging and needs to catch-up
		LogItem* li = logCache.Get(PaxosLearner::msg.paxosID);
		if (li != NULL)
			PaxosLearner::SendChosen(PaxosLearner::udpread.endpoint,
				PaxosLearner::msg.paxosID, li->value);
	}
}

void ReplicatedLog::OnRequest()
{
	Log_Trace();

	if (PaxosAcceptor::msg.paxosID < PaxosAcceptor::paxosID)
	{
		// the node is lagging and needs to catch-up
		LogItem* li = logCache.Get(PaxosAcceptor::msg.paxosID);
		if (li != NULL)
		{
			Endpoint endpoint = PaxosAcceptor::udpread.endpoint;
			endpoint.SetPort(endpoint.GetPort() - PAXOS_PROPOSER_PORT_OFFSET
												+ PAXOS_LEARNER_PORT_OFFSET);
			PaxosLearner::SendChosen(endpoint, PaxosAcceptor::msg.paxosID, li->value);
		}
	}
	else // paxosID < msg.paxosID
	{
		//	I am lagging and need to catch-up
		
		if (!catchupTimeout.IsActive())
			scheduler->Add(&catchupTimeout);
		
		Endpoint endpoint = PaxosAcceptor::udpread.endpoint;
		endpoint.SetPort(endpoint.GetPort() - PAXOS_PROPOSER_PORT_OFFSET
											+ PAXOS_LEARNER_PORT_OFFSET);
		PaxosLearner::RequestChosen(endpoint);
	}
}

void ReplicatedLog::NewPaxosRound()
{
	PaxosProposer::scheduler->Remove(&(PaxosProposer::prepareTimeout));
	PaxosProposer::scheduler->Remove(&(PaxosProposer::proposeTimeout));
	PaxosProposer::paxosID++;
	PaxosProposer::state.Init();

	PaxosAcceptor::paxosID++;
	PaxosAcceptor::state.Init();

	PaxosLearner::paxosID++;
	PaxosLearner::state.Init();
}

void ReplicatedLog::OnCatchupTimeout()
{
	Log_Trace();

	if (replicatedDB != NULL)
		replicatedDB->OnDoCatchup();
}

void ReplicatedLog::OnLearnLease()
{
	if (replicatedDB)
		replicatedDB->OnMasterLease(masterLease.LeaseOwner());
}

void ReplicatedLog::OnLeaseTimeout()
{
	if (replicatedDB)
		replicatedDB->OnMasterLeaseExpired();
}
