#include "PaxosLearner.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "System/Log.h"
#include "PaxosConsts.h"

PaxosLearner::PaxosLearner() :
	onRead(this, &PaxosLearner::OnRead),
	onWrite(this, &PaxosLearner::OnWrite)
{
}

bool PaxosLearner::Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_)
{
	// I/O framework
	ioproc = ioproc_;
	scheduler = scheduler_;
	config = config_;
	
	state.Init();
	
	if (!socket.Create(UDP)) return false;
	if (!socket.Bind(config->port + PAXOS_LEARNER_PORT_OFFSET)) return false;
	if (!socket.SetNonblocking()) return false;
	
	udpread.fd = socket.fd;
	udpread.data = rdata;
	udpread.onComplete = &onRead;
	
	udpwrite.fd = socket.fd;
	udpwrite.data = wdata;
	udpwrite.onComplete = &onWrite;

	return ioproc->Add(&udpread);
}

void PaxosLearner::OnRead()
{
	Log_Trace();
		
	Log_Message("Participant %d received message %.*s from %s",
		config->nodeID, udpread.data.length, udpread.data.buffer, udpread.endpoint.ToString());
	
	if (!msg.Read(udpread.data))
		Log_Message("PaxosLeaseMsg::Read() failed: invalid message format");
	else
		ProcessMsg();
	
	ioproc->Add(&udpread);
	
	assert(udpread.active);
}

void PaxosLearner::OnWrite()
{
	Log_Trace();
}

bool PaxosLearner::RequestChosen(Endpoint endpoint)
{
	Log_Trace();
	
	if (udpwrite.active)
		return false;
		
	msg.RequestChosen(paxosID);
	
	if (!msg.Write(udpwrite.data))
		return false;
	
	udpwrite.endpoint = endpoint;
	
	Log_Message("Participant %d sending message %.*s to %s",
		config->nodeID, udpwrite.data.length, udpwrite.data.buffer,
		udpwrite.endpoint.ToString());
	
	ioproc->Add(&udpwrite);
	
	return true;
}

bool PaxosLearner::SendChosen(Endpoint endpoint, ulong64 paxosID, ByteString& value)
{
	Log_Trace();
	
	if (udpwrite.active)
		return false;
		
	msg.LearnChosen(paxosID, value);
	
	if (!msg.Write(udpwrite.data))
		return false;
	
	udpwrite.endpoint = endpoint;
	
	Log_Message("Participant %d sending message %.*s to %s",
		config->nodeID, udpwrite.data.length, udpwrite.data.buffer,
		udpwrite.endpoint.ToString());
	
	ioproc->Add(&udpwrite);
	
	return true;
}

void PaxosLearner::ProcessMsg()
{
	if (msg.type == LEARN_CHOSEN)
		OnLearnChosen();
	else if (msg.type == REQUEST_CHOSEN)
		OnRequestChosen();
	else
		ASSERT_FAIL();
}

void PaxosLearner::OnLearnChosen()
{
	Log_Trace();
	
	state.learned = true;
	state.value.Set(msg.value);

	Log_Message("+++ Consensus for paxosID = %llu is %.*s +++", paxosID,
		state.value.length, state.value.buffer);
}

void PaxosLearner::OnRequestChosen()
{
	Log_Trace();

	if (state.learned)
		SendChosen(udpread.endpoint, paxosID, state.value);
}

bool PaxosLearner::Learned()
{
	return state.learned;	
}

ByteString PaxosLearner::Value()
{
	return state.value;
}
