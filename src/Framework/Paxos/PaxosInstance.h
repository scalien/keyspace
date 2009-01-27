#ifndef PAXOSINSTANCE_H
#define PAXOSINSTANCE_H

#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"
#include "PaxosMsg.h"
#include "PaxosState.h"
#include "PaxosConfig.h"

#define PAXOS_TIMEOUT	100

class PaxosInstance
{
public:
	IOProcessor*		ioproc;
	Scheduler*			scheduler;
	Socket				socket;

	UDPRead				udpread;
	UDPWrite			udpwrite;
	
	ByteArray<1024>		rdata;
	ByteArray<1024>		wdata;
	
	PaxosConfig			config;

						PaxosInstance();
	
	bool				Start(ByteString value);

	bool				Init(IOProcessor* ioproc_, Scheduler* scheduler_);
	
	bool				ReadConfig(char* filename) { return config.Read(filename); }
	
	bool				SendMsg();
	void				SendMsgToAll();
		
	void				Reset();
	
	void				OnRead();
	void				OnWrite();

	MFunc<PaxosInstance>readCallable;
	MFunc<PaxosInstance>writeCallable;

		
// events:
	void				ProcessMsg();
	
	virtual void		OnPrepareRequest();
	virtual void		OnPrepareResponse();
	virtual void		OnProposeRequest();
	virtual void		OnProposeResponse();
	virtual void		OnLearnChosen();
	
	void				StopPreparer();
	void				StopProposer();

	void				Prepare();
	bool				TryFinishPrepare();
	void				Propose();
	bool				TryFinishPropose();
	
// timeout related:
	void				OnPrepareTimeout();
	void				OnProposeTimeout();
	MFunc<PaxosInstance>onPrepareTimeout;
	MFunc<PaxosInstance>onProposeTimeout;
	Timer				prepareTimeout;
	Timer				proposeTimeout;

// single paxos specific:
	int					paxosID;
	long				highest_promised;

	bool				sendtoall;
	Endpoint*			sending_to;		// todo: don't let the list change

	PaxosMsg			msg;

	PreparerState		preparer;
	ProposerState		proposer;
	AcceptorState		acceptor;
	LearnerState		learner;
		
// keeping track of messages during prepare and propose phases
	int					numSent;
	int					numReceived;
	int					numAccepted;
	int					numRejected;
};

#endif
