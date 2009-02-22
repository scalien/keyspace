#include "ReplicatedLog.h"
#include <string.h>

ReplicatedLog::ReplicatedLog()
: catchupTimeout(CATCHUP_TIMEOUT, this, &ReplicatedLog::OnCatchupTimeout)
{
}

bool ReplicatedLog::Init(IOProcessor* ioproc_, Scheduler* scheduler_, bool multiPaxos)
{
	Paxos::Init(ioproc_, scheduler_);
	appending = false;

	for (int i = 0; i < SIZE(replicatedDBs); i++)
		replicatedDBs[i] = NULL;
	
	if (multiPaxos)
		masterLease.Init(ioproc_, scheduler_, this);
		
	return true;
}

bool ReplicatedLog::PersistState(Transaction* transaction)
{
	return Paxos::PersistState(transaction);
}

bool ReplicatedLog::ReadConfig(char* filename)
{
	return Paxos::ReadConfig(filename);
}

bool ReplicatedLog::Stop()
{
	PersistState(NULL);
	
	sendtoall = false;
	
	if (udpread.active)
		ioproc->Remove(&udpread);

	if (udpwrite.active)
		ioproc->Remove(&udpwrite);
	
	for (int i = 0; i < SIZE(replicatedDBs); i++)
		if (replicatedDBs[i] != NULL)
			replicatedDBs[i]->OnStop();
	
	return true;
}

bool ReplicatedLog::Continue()
{
	ioproc->Add(&udpwrite);
	
	for (int i = 0; i < SIZE(replicatedDBs); i++)
		if (replicatedDBs[i] != NULL)
			replicatedDBs[i]->OnContinue();

	return true;
}

bool ReplicatedLog::Append(ByteString value_)
{
	if (!logQueue.Push(value_.buffer[0], value_))
		return false;
	
	if (!appending)
	{
		value = value_;
		appending = true;
		return Paxos::Start(value);
	}
	
	return true;
}

bool ReplicatedLog::RemoveAll(char protocol)
{
	logQueue.RemoveAll(protocol);
	
	return true;
}

void ReplicatedLog::Register(char protocol, ReplicatedDB* replicatedDB)
{
	replicatedDBs[protocol] = replicatedDB;
}

LogItem* ReplicatedLog::LastLogItem()
{
	return logCache.Last();
}

void ReplicatedLog::SetMaster(bool master)
{
	Log_Trace();

	proposer.leader = master;

	for (int i = 0; i < SIZE(replicatedDBs); i++)
	{
		if (replicatedDBs[i] != NULL)
		{
			if (master)
				replicatedDBs[i]->OnMaster();
			else
				replicatedDBs[i]->OnSlave();
		}
	}
	
}

bool ReplicatedLog::IsMaster()
{
	return proposer.leader;
}

int ReplicatedLog::NodeID()
{
	return config.nodeID;
}

void ReplicatedLog::OnPrepareRequest()
{
	Log_Trace();
	
	if (msg.paxosID > highest_paxosID_seen)
		highest_paxosID_seen = msg.paxosID;
	
	if (msg.paxosID == paxosID)
		return Paxos::OnPrepareRequest();

	if (msg.paxosID < paxosID)
	{
		// the node is lagging and needs to catch-up
		LogItem* li = logCache.Get(msg.paxosID);
		if (li != NULL)
		{
			if (!msg.LearnChosen(msg.paxosID, li->value))
			{
				ioproc->Add(&udpread);
				return;
			}
		}
		else
		{
			ioproc->Add(&udpread);
			return;
		}
	}
	else // paxosID < msg.paxosID
	{
		/*	I am lagging and need to catch-up
			I will send a prepare request which will
			trigger the other node to send me the chosen value
		*/
		
		if (!catchupTimeout.active)
			scheduler->Add(&catchupTimeout);
		
		msg.PrepareRequest(paxosID, 1);
	}

	udpwrite.endpoint = udpread.endpoint;
	if (!msg.Write(udpwrite.data))
	{
		ioproc->Add(&udpread);
		return;
	}
	
	Paxos::SendMsg();
}

void ReplicatedLog::OnPrepareResponse()
{
	Log_Trace();
	
	if (msg.paxosID < paxosID)
	{
		ioproc->Add(&udpread);
		return;
	}
	
	Paxos::OnPrepareResponse();
}

void ReplicatedLog::OnProposeRequest()
{
	Log_Trace();
	
	if (msg.paxosID > highest_paxosID_seen)
		highest_paxosID_seen = msg.paxosID;

	
	if (msg.paxosID == paxosID)
		return Paxos::OnProposeRequest();

	if (msg.paxosID < paxosID)
	{
		// the node is lagging and needs to catch-up
		LogItem* li = logCache.Get(msg.paxosID);
		if (li != NULL)
			msg.LearnChosen(msg.paxosID, li->value); // todo: retval
	}
	else // paxosID < msg.paxosID
	{
		/*	I am lagging and need to catch-up
			I will send a prepare request which will
			trigger the other node to send me the chosen value
		*/
		
		if (!catchupTimeout.active)
			scheduler->Add(&catchupTimeout);
		
		msg.PrepareRequest(paxosID, 1);
	}

	udpwrite.endpoint = udpread.endpoint;
	if (!msg.Write(udpwrite.data))
	{
		ioproc->Add(&udpread);
		return;
	}
	Paxos::SendMsg();
}

void ReplicatedLog::OnProposeResponse()
{
	Log_Trace();
	
	if (msg.paxosID < paxosID)
	{
		ioproc->Add(&udpread);
		return;
	}
	
	Paxos::OnProposeResponse();
}

void ReplicatedLog::OnLearnChosen()
{
	Log_Trace();
	
	if (msg.paxosID > highest_paxosID_seen)
		highest_paxosID_seen = msg.paxosID;

	
	char protocol;
	bool myappend;
	
	 myappend = false;
	
	if (msg.paxosID < paxosID || (msg.paxosID == paxosID && acceptor.learned))
	{
		ioproc->Add(&udpread);
		return;
	}

	if (msg.paxosID == paxosID)
	{
		if (catchupTimeout.active)
			scheduler->Remove(&catchupTimeout);
		
		Paxos::OnLearnChosen();
		logCache.Push(paxosID, acceptor.value);
		
		ByteString acceptedValue = acceptor.value; // Paxos::Reset() will clear acceptor.value
		// TODO: is this extra variable nessecary?
		
		paxosID++;
		Paxos::Reset(); // in new round of paxos
		
		if (highest_paxosID_seen > msg.paxosID)
		{
			// quickly catch up
			msg.PrepareRequest(paxosID, 1);
			udpwrite.endpoint = udpread.endpoint;
			Paxos::SendMsg();
			return;
		}
		
		if (appending && value == acceptedValue)
		{
			myappend = true;
			logQueue.Pop();
		}
		
		if (acceptedValue.length > 0)
		{
			//Transaction tx(table);
			
			protocol = acceptedValue.buffer[0];

			if (replicatedDBs[protocol] != NULL)
				replicatedDBs[protocol]->OnAppend(/*&tx*/ NULL, LastLogItem());
			
			//tx.Commit();
		}
				
		if (logQueue.Size() > 0)
		{
			value = logQueue.Next()->value;
			appending = true;
			Paxos::Start(value);
			return;
		}
		else
		{
			appending = false;
		}
	}
	else if (msg.paxosID > paxosID)
	{
		/*	I am lagging and need to catch-up
			I will send a prepare request which will
			trigger the other node to send me the chosen value
		*/
		
		highest_paxosID_seen = msg.paxosID;
		
		if (!catchupTimeout.active)
			scheduler->Add(&catchupTimeout);
		
		msg.PrepareRequest(paxosID, 0);
		udpwrite.endpoint = udpread.endpoint;
		if (!msg.Write(udpwrite.data))
		{
			ioproc->Add(&udpread);
			return;
		}
		Paxos::SendMsg();
		return;
	}

	ioproc->Add(&udpread);
}

void ReplicatedLog::OnCatchupTimeout()
{
	Log_Trace();

	for (int i = 0; i < SIZE(replicatedDBs); i++)
		if (replicatedDBs[i] != NULL)
			replicatedDBs[i]->OnDoCatchup();
}
