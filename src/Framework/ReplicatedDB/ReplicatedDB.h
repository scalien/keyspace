#ifndef REPLICATEDDB_H
#define REPLICATEDDB_H

#include "Framework/Database/Transaction.h"
#include "Framework/ReplicatedLog/LogQueue.h"

class Entry;

class ReplicatedDB
{
public:
	virtual ~ReplicatedDB() {}
	
	virtual void OnAppend(Transaction* transaction, uint64_t paxosID,
					ByteString value, bool ownAppend) = 0;
	
	virtual void OnMasterLease(unsigned nodeID) = 0;
	virtual void OnMasterLeaseExpired() = 0;
	
	virtual void OnDoCatchup(unsigned nodeID) = 0;
};

#endif
