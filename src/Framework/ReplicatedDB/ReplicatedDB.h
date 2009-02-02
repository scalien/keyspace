#ifndef REPLICATEDDB_H
#define REPLICATEDDB_H

#include "Framework/Database/Transaction.h"

class Entry;

class ReplicatedDB
{
public:
	virtual void OnDoCatchup() = 0;
	
	virtual void OnAppend(Transaction* transaction, Entry* entry) = 0;
};

#endif