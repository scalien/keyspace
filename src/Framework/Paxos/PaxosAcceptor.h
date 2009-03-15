#ifndef PAXOSACCEPTOR_H
#define PAXOSACCEPTOR_H

#include "System/Common.h"
#include "System/Events/Scheduler.h"
#include "Framework/Transport/TransportWriter.h"
#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "Framework/Database/Transaction.h"
#include "PaxosMsg.h"
#include "PaxosState.h"
#include "PaxosConfig.h"

#define ACCEPTOR_PORT_OFFSET 1

class PaxosAcceptor
{
friend class ReplicatedLog;

public:
	PaxosAcceptor();
	
	void					Init(TransportWriter** writer_, Scheduler* scheduler_, PaxosConfig* config_);
	
	void					SetPaxosID(ulong64 paxosID_);

protected:
	bool					ReadState();
	bool					WriteState(Transaction* transaction);
	
	void					SendReply(unsigned nodeID);

	void					OnPrepareRequest(PaxosMsg& msg_);
	void					OnProposeRequest(PaxosMsg& msg_);

	void					OnDBComplete();

	TransportWriter**		writers;
	Scheduler*				scheduler;

	ByteArray<PAXOS_BUFSIZE>wdata;
			
// single paxos specific:
	ulong64					paxosID;
	PaxosMsg				msg;
	
	unsigned				senderID;
	PaxosConfig*			config;
	PaxosAcceptorState		state;

// datebase:
	Table*					table;
	Transaction				transaction;
	MultiDatabaseOp			mdbop;
	ByteArray<128>			bytearrays[4];
	ulong64					writtenPaxosID;

	MFunc<PaxosAcceptor>	onDBComplete;
};

#endif
