#ifndef PAXOSLEARNER_H
#define PAXOSLEARNER_H

#include "System/Common.h"
#include "System/Events/Scheduler.h"
#include "Framework/Transport/TransportWriter.h"
#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "Framework/Database/Transaction.h"
#include "PaxosMsg.h"
#include "PaxosState.h"
#include "PaxosConfig.h"

class PaxosLearner
{
friend class ReplicatedLog;

protected:
	PaxosLearner();
	
	void					Init(TransportWriter** writers_, Scheduler* scheduler_);
	
	bool					RequestChosen(unsigned nodeID);
	bool					SendChosen(unsigned nodeID, ulong64 paxosID, ByteString& value);
	
	bool					Learned();
	ByteString				Value();
	
	void					SetPaxosID(ulong64 paxosID_);

protected:
	void					OnLearnChosen(PaxosMsg& msg_);
	void					OnRequestChosen(PaxosMsg& msg_);

	TransportWriter**		writers;
	Scheduler*				scheduler;

	ByteArray<PAXOS_BUFSIZE>wdata;
	
	ulong64					paxosID;
	
	PaxosMsg				msg;
	
	PaxosConfig*			config;
	
	PaxosLearnerState		state;
	
};

#endif
