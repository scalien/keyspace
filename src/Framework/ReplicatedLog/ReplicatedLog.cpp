#include "ReplicatedLog.h"
#include <string.h>

bool ReplicatedLog::Init(IOProcessor* ioproc_, Scheduler* scheduler_)
{
	PaxosInstance::Init(ioproc_, scheduler_);
	appending = false;
	return true;
}

bool ReplicatedLog::Append(ByteString value_)
{
	if (!appending)
	{
		value = value_;
		appending = true;
		return PaxosInstance::Start(value);
	}
	else
		return false;
}

bool ReplicatedLog::Remove()
{
	appending = false;
	
	return true;
}

void ReplicatedLog::OnPrepareRequest()
{
	Log_Trace();
	
	if (msg.paxosID == paxosID)
		return PaxosInstance::OnPrepareRequest();

	if (msg.paxosID < paxosID)
	{
		// the node is lagging and needs to catch-up
		Entry* record = memlog.Get(msg.paxosID);
		if (record != NULL)
			msg.LearnChosen(msg.paxosID, record->value);
	}
	else
	{
		/*	I am lagging and need to catch-up
			I will send a prepare request which will
			trigger the other node to send me the chosen value
		*/
		msg.PrepareRequest(paxosID, 1);
	}

	udpwrite.endpoint = udpread.endpoint;
	PaxosMsg::Write(&msg, udpwrite.data); // todo: retval
	PaxosInstance::SendMsg();
}

void ReplicatedLog::OnPrepareResponse()
{
	Log_Trace();
	
	if (msg.paxosID < paxosID)
	{
		ioproc->Add(&udpread);
		return;
	}
	
	PaxosInstance::OnPrepareResponse();
}

void ReplicatedLog::OnProposeRequest()
{
	Log_Trace();
	
	if (msg.paxosID == paxosID)
		return PaxosInstance::OnProposeRequest();

	if (msg.paxosID < paxosID)
	{
		// the node is lagging and needs to catch-up
		Entry* record = memlog.Get(msg.paxosID);
		if (record != NULL)
			msg.LearnChosen(msg.paxosID, record->value); // todo: retval
	}
	else
	{
		/*	I am lagging and need to catch-up
			I will send a prepare request which will
			trigger the other node to send me the chosen value
		*/
		msg.PrepareRequest(paxosID, 1);
	}

	udpwrite.endpoint = udpread.endpoint;
	PaxosMsg::Write(&msg, udpwrite.data); // todo: retval
	PaxosInstance::SendMsg();
}

void ReplicatedLog::OnProposeResponse()
{
	Log_Trace();
	
	if (msg.paxosID < paxosID)
	{
		ioproc->Add(&udpread);
		return;
	}
	
	PaxosInstance::OnProposeResponse();
}

void ReplicatedLog::OnLearnChosen()
{
	bool myappend = false;
	
	if (msg.paxosID < paxosID || (msg.paxosID == paxosID && learner.learned))
	{
		ioproc->Add(&udpread);
		return;
	}

	if (msg.paxosID == paxosID)
	{
		PaxosInstance::OnLearnChosen();
		memlog.Push(paxosID, learner.value);
		paxosID++;
		PaxosInstance::Reset(); // in new round of paxos
		
		if (appending && value == learner.value)
			myappend = true;
		
		Call(appendCallback); // application must be notified of *all* log appends
		
		// see if the chosen value is my application's
		if (appending)
		{
			if (myappend)
				appending = false;
			else
			{
				// my application's value has not yet been chosen, start another round of paxos
				PaxosInstance::Start(value);
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
		msg.PrepareRequest(paxosID, 1);
		udpwrite.endpoint = udpread.endpoint;
		PaxosMsg::Write(&msg, udpwrite.data); // todo: retval
		PaxosInstance::SendMsg();
		return;
	}

	ioproc->Add(&udpread);
}
