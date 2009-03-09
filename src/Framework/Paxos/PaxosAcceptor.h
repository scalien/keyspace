#ifndef PAXOSACCEPTOR_H
#define PAXOSACCEPTOR_H

#include "System/Common.h"
#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"
#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "Framework/Database/Transaction.h"
#include "PaxosMsg.h"
#include "PaxosState.h"
#include "PaxosConfig.h"

#define ACCEPTOR_PORT_OFFSET 1

class PaxosAcceptor
{
public:
							PaxosAcceptor();
	
	bool					Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_);
	
	void					OnRead();
	void					OnWrite();

	void					SetPaxosID(ulong64 paxosID_);

protected:
	bool					ReadState();
	bool					WriteState(Transaction* transaction);
	
	void					SendReply();

	void					ProcessMsg();	
	virtual void			OnPrepareRequest();
	virtual void			OnProposeRequest();

	void					OnDBComplete();

	IOProcessor*			ioproc;
	Scheduler*				scheduler;
	Socket					socket;

	UDPRead					udpread;
	UDPWrite				udpwrite;
	
	ByteArray<64*KB>		rdata;
	ByteArray<64*KB>		wdata;
			
	MFunc<PaxosAcceptor>	onRead;
	MFunc<PaxosAcceptor>	onWrite;
	
// single paxos specific:
	ulong64					paxosID;
	PaxosMsg				msg;
	
	PaxosConfig*			config;
	PaxosAcceptorState		state;

// datebase:
	Table*					table;
	Transaction				transaction;
	MultiDatabaseOp			mdbop;
	ByteArray<VALUE_SIZE>	bytearrays[4];
	ulong64					writtenPaxosID;

	MFunc<PaxosAcceptor>	onDBComplete;
};

#endif
