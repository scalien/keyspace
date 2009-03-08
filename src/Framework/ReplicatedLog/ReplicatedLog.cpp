#include "ReplicatedLog.h"
#include <string.h>
#include "PaxosConsts.h"

ReplicatedLog::ReplicatedLog()
:	onCatchupTimeout(this, &ReplicatedLog::OnCatchupTimeout),
	catchupTimeout(CATCHUP_TIMEOUT, &onCatchupTimeout)
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
		value = value_;
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

void ReplicatedLog::SetMaster(bool master)
{
	Log_Trace();

	PaxosProposer::state.leader = master;

	if (replicatedDB != NULL)
	{
		if (master)
			replicatedDB->OnMaster();
		else
			replicatedDB->OnSlave();
	}
}

bool ReplicatedLog::IsMaster()
{
	return  (config.nodeID == 0);
//	return PaxosProposer::state.leader; // TODO
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
	
	if (PaxosAcceptor::msg.paxosID > highestPaxosID)
		highestPaxosID = PaxosAcceptor::msg.paxosID;
	
	if (PaxosAcceptor::msg.paxosID == PaxosAcceptor::paxosID)
		return PaxosAcceptor::OnProposeRequest();

	OnRequest();
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
	
	if (PaxosLearner::msg.paxosID > highestPaxosID)
		highestPaxosID = PaxosLearner::msg.paxosID;

	bool myappend;
	
	 myappend = false;
	
	if (PaxosLearner::msg.paxosID == PaxosLearner::paxosID)
	{
		if (catchupTimeout.IsActive())
			scheduler->Remove(&catchupTimeout);
		
		PaxosLearner::OnLearnChosen();
		logCache.Push(PaxosLearner::paxosID, PaxosLearner::state.value);
		
		//ByteString acceptedValue = acceptor.value; // Paxos::Reset() will clear acceptor.value
		// TODO: is this extra variable nessecary?
		
		// TODO: increment paxos state
		PaxosProposer::paxosID++;
		PaxosProposer::state.Init();
		PaxosAcceptor::paxosID++;
		PaxosAcceptor::state.Init();
		PaxosLearner::paxosID++;
		PaxosLearner::state.Init();
		
		if (highestPaxosID > PaxosLearner::msg.paxosID)
		{
			Endpoint endpoint = PaxosLearner::udpread.endpoint;
			endpoint.SetPort(endpoint.GetPort() - PAXOS_PROPOSER_PORT_OFFSET
												+ PAXOS_LEARNER_PORT_OFFSET);
			PaxosLearner::RequestChosen(endpoint);
			return;
		}
		
		if (appending && value == LastLogItem()->value/*acceptedValue*/)
		{
			myappend = true;
			logQueue.Pop();
		}
		
		if (/*acceptedValue*/LastLogItem()->value.length > 0)
		{
			//Transaction tx(table);
			
			if (replicatedDB != NULL)
				replicatedDB->OnAppend(NULL, LastLogItem());
			
			//tx.Commit();
		}
				
		if (logQueue.Size() > 0)
		{
			value = logQueue.Next()->value;
			appending = true;
			PaxosProposer::Propose(value);
			return;
		}
		else
		{
			appending = false;
		}
	}
	else if (PaxosLearner::msg.paxosID > PaxosLearner::paxosID)
	{
		/*	I am lagging and need to catch-up
			I will send a prepare request which will
			trigger the other node to send me the chosen value
		*/
		
		if (PaxosLearner::msg.paxosID > highestPaxosID)
			highestPaxosID = PaxosLearner::msg.paxosID;
		
		if (!catchupTimeout.IsActive())
			scheduler->Add(&catchupTimeout);
		
			Endpoint endpoint = PaxosLearner::udpread.endpoint;
			endpoint.SetPort(endpoint.GetPort() - PAXOS_PROPOSER_PORT_OFFSET
												+ PAXOS_LEARNER_PORT_OFFSET);
			PaxosLearner::RequestChosen(endpoint);
			return;
	}
	else if (PaxosLearner::msg.paxosID < PaxosLearner::paxosID ||
		(PaxosLearner::msg.paxosID == PaxosLearner::paxosID && PaxosLearner::state.learned))
	{
		PaxosLearner::ioproc->Add(&(PaxosLearner::udpread));
		return;
	}

	PaxosLearner::ioproc->Add(&(PaxosLearner::udpread));
}

void ReplicatedLog::OnRequestChosen()
{
}

void ReplicatedLog::OnRequest()
{
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
			return;
		}
		else
		{
			PaxosAcceptor::ioproc->Add(&(PaxosAcceptor::udpread));
			return;
		}
	}
	else // paxosID < msg.paxosID
	{
		/*	I am lagging and need to catch-up
		*/
		
		if (!catchupTimeout.IsActive())
			scheduler->Add(&catchupTimeout);
		
		Endpoint endpoint = PaxosAcceptor::udpread.endpoint;
		endpoint.SetPort(endpoint.GetPort() - PAXOS_PROPOSER_PORT_OFFSET
											+ PAXOS_LEARNER_PORT_OFFSET);
		PaxosLearner::RequestChosen(endpoint);
	}
}

void ReplicatedLog::OnCatchupTimeout()
{
	Log_Trace();

	if (replicatedDB != NULL)
		replicatedDB->OnDoCatchup();
}
