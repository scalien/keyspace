#include "PaxosConfig.h"
#include <stdlib.h>
#include <math.h>
#include "Framework/Database/Table.h"
#include "Framework/Database/Transaction.h"
#include "PaxosConsts.h"

bool PaxosConfig::Init(char *filename)
{
	FILE*		f;
	char		buffer[1024];
	Endpoint	endpoint, me;

#define ReadLine()	if (fgets(buffer, sizeof(buffer), f) == NULL) \
					{ Log_Message("fgets() failed"); return false; }

	f = fopen(filename, "r");
	if (f == NULL)
	{
		Log_Errno();
		return false;
	}
	
	// first line is my nodeID
	ReadLine();
	
	nodeID = -1;
	nodeID = atoi(buffer);
	if (nodeID < 0 || nodeID > 1000)
	{
		Log_Message("atoi() failed to produce a sensible value");
		return false;
	}
	
	// next line is my host specification
	ReadLine();
	
	me.Set(buffer);
	port = me.GetPort();
	
	// read empty line
	ReadLine();
	
	// now read other nodes, including myself
	while (fgets(buffer, sizeof(buffer), f) != NULL)
	{
		endpoint.Set(buffer);
		if (endpoint == me)
			endpoints.Append(endpoint);
		else
			endpoints.Add(endpoint);
	}
	
	numNodes = endpoints.Size();
	
	InitRestartCounter();
	
	return true;
}

void PaxosConfig::InitRestartCounter()
{
	Log_Trace();
	
	bool			ret;
	int				nread;
	ByteArray<32>	baRestartCounter;
	Table*			table;
	
	table = database.GetTable("state");
	if (table == NULL)
	{
		restartCounter = 0;
		return;
	}
	
	Transaction tx(table);
	tx.Begin();
	
	ret = table->Get(&tx, "restartCounter", baRestartCounter);

	if (ret)
	{
		restartCounter = strntoulong64(baRestartCounter.buffer, baRestartCounter.length, &nread);
		if (nread != baRestartCounter.length)
			restartCounter = 0;
	}
	else
		restartCounter = 0;
	
	restartCounter++;
	
	baRestartCounter.length =
		snprintf(baRestartCounter.buffer, baRestartCounter.size, "%llu", restartCounter);
	
	table->Put(&tx, "restartCounter", baRestartCounter);
	tx.Commit();
	
	Log_Message("Running with restartCounter = %llu", restartCounter);
}

int PaxosConfig::MinMajority()
{
	return (floor(numNodes / 2) + 1);
}

ulong64 PaxosConfig::NextHighest(ulong64 proposalID)
{
	// <proposal count since restart> <restartCounter> <nodeID>
	
	ulong64 left, middle, right, nextProposalID;
	
	left = proposalID >> (WIDTH_NODEID + WIDTH_RESTART_COUNTER);
	
	left++;
	
	left = left << (WIDTH_NODEID + WIDTH_RESTART_COUNTER);

	middle = restartCounter << WIDTH_NODEID;

	right = nodeID;

	nextProposalID = left | middle | right;
	
	return nextProposalID;
}
