#include "TestDB.h"

#define PROTOCOL_TESTDB		'T'

void TestDB::Init(IOProcessor* ioproc_, Scheduler* scheduler_, ReplicatedLog* replicatedLog_)
{
	replicatedLog = replicatedLog_;
	seq = 0;
	
	replicatedLog->Register(PROTOCOL_TESTDB, this);
	
	OnAppend(NULL, NULL);
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

void TestDB::OnMaster()
{
	OnAppend(NULL, NULL);
}