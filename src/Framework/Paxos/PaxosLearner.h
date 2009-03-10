#ifndef PAXOSLEARNER_H
#define PAXOSLEARNER_H

#include "System/Common.h"
#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"
#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "Framework/Database/Transaction.h"
#include "PaxosMsg.h"
#include "PaxosState.h"
#include "PaxosConfig.h"

class PaxosLearner
{
protected:
							PaxosLearner();
	
	bool					Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_);
	
	void					OnRead();
	void					OnWrite();
	
	bool					RequestChosen(Endpoint endpoint);
	bool					SendChosen(Endpoint endpoint, ulong64 paxosID, ByteString& value);
	
	bool					Learned();
	ByteString				Value();
	
	void					SetPaxosID(ulong64 paxosID_);
	
	void					SetOnLearnValue(Callable* onLearnValueCallback_);

protected:
	void					ProcessMsg();
	virtual void			OnLearnChosen();
	virtual void			OnRequestChosen();

	IOProcessor*			ioproc;
	Scheduler*				scheduler;
	Socket					socket;

	UDPRead					udpread;
	UDPWrite				udpwrite;
	
	ByteArray<64*KB>		rdata;
	ByteArray<64*KB>		wdata;
	
	MFunc<PaxosLearner>		onRead;
	MFunc<PaxosLearner>		onWrite;
	
	ulong64					paxosID;
	
	PaxosMsg				msg;
	
	PaxosConfig*			config;
	
	PaxosLearnerState		state;
	
	Callable*				onLearnValueCallback;
};

#endif
