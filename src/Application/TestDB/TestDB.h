#ifndef TESTDB_H
#define TESTDB_H

#include "System/IO/IOProcessor.h"
#include "System/Events/Scheduler.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Framework/ReplicatedDB/ReplicatedDB.h"

class TestDB : public ReplicatedDB
{
public:
	void Init(IOProcessor* ioproc_, Scheduler* scheduler_, ReplicatedLog* replicatedLog_);

	void OnAppend(Transaction*, ulong64, ByteString, bool);
	
	void OnMasterLease(unsigned nodeID);
	
	void OnMasterLeaseExpired()		{ /* empty */ };
	
	void OnDoCatchup()				{ /* empty */ };
	
private:
	ReplicatedLog*	replicatedLog;
	int				seq;
	ByteArray<1024>	ba;
	
	bool			master;
};

#endif
