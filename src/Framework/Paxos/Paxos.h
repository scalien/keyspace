#ifndef PAXOS_H
#define PAXOS_H

#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"
#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "Framework/Database/Transaction.h"
#include "PaxosMsg.h"
#include "PaxosState.h"
#include "PaxosConfig.h"

#define PAXOS_TIMEOUT	500 // TODO: I increased this for testing

class Paxos
{
public:
	IOProcessor*			ioproc;
	Scheduler*				scheduler;
	Socket					socket;

	UDPRead					udpread;
	UDPWrite				udpwrite;
	
	ByteArray<1024>			rdata; // TODO: do I need two of these?
	ByteArray<1024>			wdata; // TODO: this size should be matched to the one in PaxosMsg.h
	
	PaxosConfig				config;

							Paxos();
	
	bool					Start(ByteString value);

	bool					Init(IOProcessor* ioproc_, Scheduler* scheduler_);
	
	bool					ReadConfig(char* filename) { return config.Read(filename); }
	
	bool					ReadState();
	bool					WriteState(Transaction* transaction);
	
	bool					SendMsg();
	void					SendMsgToAll();
		
	void					Reset();
	
	void					OnRead();
	void					OnWrite();

	MFunc<Paxos>			onRead;
	MFunc<Paxos>			onWrite;
		
// events:
	void					ProcessMsg();
	
	virtual void			OnPrepareRequest();
	virtual void			OnPrepareResponse();
	virtual void			OnProposeRequest();
	virtual void			OnProposeResponse();
	virtual void			OnLearnChosen();
	
	void					StopPreparer();
	void					StopProposer();

	void					Prepare();
	bool					TryFinishPrepare();
	void					Propose();
	bool					TryFinishPropose();
	
// timeout related:
	void					OnPrepareTimeout();
	void					OnProposeTimeout();
	MFunc<Paxos>			onPrepareTimeout;
	MFunc<Paxos>			onProposeTimeout;
	Timer					prepareTimeout;
	Timer					proposeTimeout;

// single paxos specific:
	ulong64					paxosID;

	bool					sendtoall;
	Endpoint*				sending_to;

	PaxosMsg				msg;

	ProposerState			proposer;
	AcceptorState			acceptor;
		
// keeping track of messages during prepare and propose phases
	int						numSent;
	int						numReceived;
	int						numAccepted;
	int						numRejected;
	
	Table*					table;
	Transaction				transaction;
	MultiDatabaseOp			dbop;
	ByteArray<VALUE_SIZE>	bytearrays[4];
	void					OnDBComplete();
	MFunc<Paxos>			onDBComplete;
};

#endif
