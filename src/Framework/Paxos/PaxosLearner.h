#ifndef PAXOSLEARNER_H
#define PAXOSLEARNER_H

#include "System/Common.h"
#include "Framework/Transport/TransportWriter.h"
#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "Framework/Database/Transaction.h"
#include "PaxosMsg.h"
#include "PaxosState.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"

class PaxosLearner
{
friend class ReplicatedLog;

protected:
	PaxosLearner();
	
	void					Init(TransportWriter** writers_);
	
	bool					RequestChosen(unsigned nodeID);
	bool					SendChosen(unsigned nodeID, uint64_t paxosID, ByteString& value);	
	bool					Learned();
	ByteString				Value();
	void					SetPaxosID(uint64_t paxosID_);

protected:
	void					OnLearnChosen(PaxosMsg& msg_);
	void					OnRequestChosen(PaxosMsg& msg_);

	TransportWriter**		writers;
	ByteBuffer				wdata;
	uint64_t				paxosID;
	PaxosMsg				msg;
	PaxosLearnerState		state;
	
};

#endif
