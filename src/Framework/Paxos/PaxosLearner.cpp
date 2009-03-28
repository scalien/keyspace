#include "PaxosLearner.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "PaxosConsts.h"

PaxosLearner::PaxosLearner()
{
}

void PaxosLearner::Init(TransportWriter** writers_, Scheduler* scheduler_)
{
	// I/O framework
	writers = writers_;
	scheduler = scheduler_;
	
	state.Init();
}

bool PaxosLearner::RequestChosen(unsigned nodeID)
{
	Log_Trace();
	
	msg.RequestChosen(paxosID, config->nodeID);
	
	msg.Write(wdata);

	writers[nodeID]->Write(wdata);
	
	return true;
}

bool PaxosLearner::SendChosen(unsigned nodeID, ulong64 paxosID, ByteString& value)
{
	Log_Trace();
	
	msg.LearnChosen(paxosID, config->nodeID, LEARN_VALUE, value);
	
	msg.Write(wdata);

	writers[nodeID]->Write(wdata);
	
	return true;
}

void PaxosLearner::OnLearnChosen(PaxosMsg& msg_)
{
	Log_Trace();
	
	msg = msg_;

	state.learned = true;
	state.value.Set(msg.value);

	Log_Message("+++ Consensus for paxosID = %llu is %.*s +++", paxosID,
		state.value.length, state.value.buffer);
}

void PaxosLearner::OnRequestChosen(PaxosMsg& msg_)
{
	Log_Trace();

	msg = msg_;

	if (state.learned)
		SendChosen(msg.nodeID, paxosID, state.value);
}

bool PaxosLearner::Learned()
{
	return state.learned;	
}

ByteString PaxosLearner::Value()
{
	return state.value;
}
