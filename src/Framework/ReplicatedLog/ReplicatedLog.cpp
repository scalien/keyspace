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
		
	return true;
}

bool ReplicatedLog::Continue()
{
	ioproc->Add(&udpwrite);

	return true;
}

bool ReplicatedLog::Append(ByteString value_)
{
	if (!appending)
	{
		value = value_;
		appending = true;
		return Paxos::Start(value);
	}
	else
		return false;
}

bool ReplicatedLog::Cancel()
{
	appending = false;
	return true;
}

void ReplicatedLog::SetReplicatedDB(ReplicatedDB* replicatedDB_)
{
	replicatedDB = replicatedDB_;
}

Entry* ReplicatedLog::LastEntry()
{
	return logCache.Last();
}

void ReplicatedLog::SetMaster(bool master)
{
	proposer.leader = true;
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
	
	if (msg.paxosID == paxosID)
		return Paxos::OnPrepareRequest();

	if (msg.paxosID < paxosID)
	{
		// the node is lagging and needs to catch-up
		Entry* record = logCache.Get(msg.paxosID);
		if (record != NULL)
		{
			if (!msg.LearnChosen(msg.paxosID, record->value))
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
	
	if (msg.paxosID == paxosID)
		return Paxos::OnProposeRequest();

	if (msg.paxosID < paxosID)
	{
		// the node is lagging and needs to catch-up
		Entry* record = logCache.Get(msg.paxosID);
		if (record != NULL)
			msg.LearnChosen(msg.paxosID, record->value); // todo: retval
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
	bool myappend = false;
	
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
		paxosID++;
		Paxos::Reset(); // in new round of paxos
		
		if (appending && value == acceptor.value)
			myappend = true;
		
		if (value.length > 0)
		{
			Transaction tx(table);
			
			if (value.buffer[0] == PROTOCOL_MASTERLEASE)
				masterLease.OnAppend(&tx, LastEntry());
			else
				replicatedDB->OnAppend(&tx, LastEntry());
			
			tx.Commit();
		}
		
		// see if the chosen value is my application's
		if (appending)
		{
			if (myappend)
				appending = false;
			else
			{
				// my application's value has not yet been chosen, start another round of paxos
				Paxos::Start(value);
				return;
			}
		}
	}
	else if (msg.paxosID > paxosID)
	{
		/*	I am lagging and need to catch-up
			I will send a prepare request which will
			trigger the other node to send me the chosen value
		*/
		
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

	if (replicatedDB)
		replicatedDB->OnDoCatchup();
}
