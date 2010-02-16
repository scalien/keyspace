#ifndef PAXOSLEARNER_H
#define PAXOSLEARNER_H

#include "System/Common.h"
#include "Framework/Transport/TransportTCPWriter.h"
#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "Framework/Database/Transaction.h"
#include "PaxosMsg.h"
#include "PaxosState.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"

class PaxosLearner
{
	friend class ReplicatedLog;
	typedef	TransportTCPWriter**	Writers;
	typedef PaxosLearnerState		State;

protected:
	PaxosLearner();
	
	void		Init(TransportTCPWriter** writers_);
	
	bool		RequestChosen(unsigned nodeID);
	bool		SendChosen(unsigned nodeID,
						   uint64_t paxosID,
						   ByteString& value);	
	bool		SendStartCatchup(unsigned nodeID,
								 uint64_t paxosID);	
	bool		Learned();
	ByteString	Value();

protected:
	void		OnLearnChosen(PaxosMsg& msg_);
	void		OnRequestChosen(PaxosMsg& msg_);

	Writers		writers;
	ByteBuffer	wdata;
	uint64_t	paxosID;
	PaxosMsg	msg;
	State		state;
};

#endif
