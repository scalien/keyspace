#include "TestDB.h"

#define PROTOCOL_TESTDB		'T'

void TestDB::Init(IOProcessor* ioproc_, Scheduler* scheduler_, ReplicatedLog* replicatedLog_)
{
	replicatedLog = replicatedLog_;
	seq = 0;
	
	replicatedLog->SetReplicatedDB(this);
}

void TestDB::OnAppend(Transaction*, LogItem*)
{
	Log_Trace();
	
	if (replicatedLog->IsMaster())
	{
		ba.length = snprintf(ba.data, ba.size,
			"%c:%d:%d", PROTOCOL_TESTDB, replicatedLog->NodeID(), seq++);
		
		replicatedLog->Append(ba);
	}
}

void TestDB::OnMasterLease(int nodeID)
{
	Log_Trace();
	
	OnAppend(NULL, NULL);
}