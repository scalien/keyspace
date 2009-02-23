#ifndef REPLICATEDDB_H
#define REPLICATEDDB_H

#include "Framework/Database/Transaction.h"
#include "Framework/ReplicatedLog/LogQueue.h"

class Entry;

class ReplicatedDB
{
public:
	virtual void OnAppend(Transaction* transaction, LogItem* entry) = 0;
	
	virtual void OnMaster() = 0;
	
	virtual void OnSlave() = 0;
	
	virtual void OnDoCatchup() = 0;
	
	virtual void OnStop() = 0;
	
	virtual void OnContinue() = 0;
};

#endif