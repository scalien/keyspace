#ifndef MASTERLEASE_H
#define MASTERLEASE_H

#include "System/Events/Scheduler.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "MasterLeaseMsg.h"

#define SEND_LEASE_EXTEND_TIMEOUT	1000
#define MASTER_TIMEOUT				2000
#define RECV_LEASE_EXTEND_TIMEOUT	3000
#define YIELD_TIMEOUT				4000

#define MASTER_UNKNOWN	-1

class MasterLease : public ReplicatedLog
{
public:
	MasterLease();

	Scheduler*			scheduler;
	
	ByteArray<1024>		data;
	ByteString			bs;
	
	MasterLeaseMsg		msg;
	
	int					epochID;
	int					masterID;
	bool				master;
	bool				designated;

	bool				Init(IOProcessor* ioproc_, Scheduler* scheduler_);
	
	void				OnAppend();
	MFunc<MasterLease>	onAppend;
	
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