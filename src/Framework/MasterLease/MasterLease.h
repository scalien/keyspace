#ifndef MASTERLEASE_H
#define MASTERLEASE_H

#include "System/IO/IOProcessor.h"
#include "System/Events/Scheduler.h"
#include "MasterLeaseMsg.h"

#define SEND_LEASE_EXTEND_TIMEOUT	1000
#define MASTER_TIMEOUT				2000
#define RECV_LEASE_EXTEND_TIMEOUT	3000
#define YIELD_TIMEOUT				4000

#define MASTER_UNKNOWN	-1

#include "Framework/Database/Transaction.h"
#include "Framework/ReplicatedLog/LogCache.h"
#include "Framework/ReplicatedDB/ReplicatedDB.h"

class ReplicatedLog;

class MasterLease : public ReplicatedDB
{
public:
	MasterLease();
		
	bool				Init(IOProcessor* ioproc_, Scheduler* scheduler_,
							ReplicatedLog* replicatedLog_);
	
	/* the ReplicatedDB interface follows */
	
	void				OnAppend(Transaction*, LogItem* entry);
	
	void				OnMaster() { /* empty */ }
	
	void				OnSlave() { /* empty */ }
	
	void				OnDoCatchup() { /* empty */ }
	
	void				OnStop() { /* TODO */ }
	
	void				OnContinue() { /* TODO */ }

private:
	ReplicatedLog*		replicatedLog;

	Scheduler*			scheduler;
	
	ByteArray<1024>		data;
	ByteString			bs;
	
	MasterLeaseMsg		msg;
	
	int					epochID;
	int					masterID;
	bool				master;
	bool				designated;

	void				Execute(ByteString command);
	
	void				ExtendLease();
	void				YieldLease();
	
	void				OnMasterTimeout();			// master
	void				OnSendLeaseExtendTimeout();	// master
	void				OnRecvLeaseExtendTimeout();	// slave
	void				OnReport();					// debugging
	
	MFunc<MasterLease>	onMasterTimeout;
	MFunc<MasterLease>	onRecvLeaseExtendTimeout;
	MFunc<MasterLease>	onSendLeaseExtendTimeout;
	MFunc<MasterLease>	onReport;
	Timer				masterTimeout;
	Timer				recvLeaseExtendTimeout;
	Timer				sendLeaseExtendTimeout;
	Timer				report;
};

#endif
