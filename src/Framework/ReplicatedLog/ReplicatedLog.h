#ifndef REPLICATEDLOG_H
#define REPLICATEDLOG_H

#include "System/Containers/List.h"
#include "System/Events/Callable.h"
#include "Framework/Paxos/PaxosInstance.h"
#include "Framework/Paxos/PaxosMsg.h"
#include "MemLog.h"

class ReplicatedLog : public PaxosInstance
{
public:
	ReplicatedLog()		{ appendCallback = NULL; }
		
	Callable*			appendCallback; // called when a new log entry is learned
	
	bool				appending;
	ByteString			value;
	
	MemLog				memlog;
	
	bool				Init(IOProcessor* ioproc_, Scheduler* scheduler_);
	
	bool				Append(ByteString value); // called by the client
	bool				Remove(); // called by the client 

// multi-paxos:
	virtual void		OnPrepareRequest();
	virtual void		OnPrepareResponse();
	virtual void		OnProposeRequest();
	virtual void		OnProposeResponse();
	virtual void		OnLearnChosen();
};

#endif