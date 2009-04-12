#include "PaxosConfig.h"
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include "System/Config.h"
#include "Framework/Database/Table.h"
#include "Framework/Database/Transaction.h"
#include "PaxosConsts.h"

PaxosConfig config;

PaxosConfig* PaxosConfig::Get()
{
	return &config;
}

bool PaxosConfig::Init()
{
	Endpoint	endpoint;

	nodeID = Config::GetIntValue("paxos.nodeID", 0);
	numNodes = Config::GetListNum("paxos.endpoints");
	for (int i = 0; i < numNodes; i++)
	{
		endpoint.Set(Config::GetListValue("paxos.endpoints", i, NULL));
		endpoints[i] = endpoint;
		if (i == nodeID)
			port = endpoint.GetPort();
	}
	
	InitRestartCounter();
	
	return true;
}

void PaxosConfig::InitRestartCounter()
{
	Log_Trace();
	
	bool			ret;
	unsigned		nread;
	ByteArray<32>	baRestartCounter;
	Table*			table;
	
	table = database.GetTable("keyspace");
	if (table == NULL)
	{
		restartCounter = 0;
		return;
	}
	
	Transaction tx(table);
	tx.Begin();
	
	ret = table->Get(&tx, "@@restartCounter", baRestartCounter);

	if (ret)
	{
		restartCounter = strntouint64_t(baRestartCounter.buffer, baRestartCounter.length, &nread);
		if (nread != baRestartCounter.length)
			restartCounter = 0;
	}
	else
		restartCounter = 0;
	
	restartCounter++;
	
	baRestartCounter.length =
		snprintf(baRestartCounter.buffer, baRestartCounter.size, "%" PRIu64 "", restartCounter);
	
	table->Set(&tx, "@@restartCounter", baRestartCounter);
	tx.Commit();
	
	Log_Message("Running with restartCounter = %" PRIu64 "", restartCounter);
}

unsigned PaxosConfig::MinMajority()
{
	return (unsigned)(floor(numNodes / 2) + 1);
}

uint64_t PaxosConfig::NextHighest(uint64_t proposalID)
{
	// <proposal count since restart> <restartCounter> <nodeID>
	
	uint64_t left, middle, right, nextProposalID;
	
	left = proposalID >> (WIDTH_NODEID + WIDTH_RESTART_COUNTER);
	
	left++;
	
	left = left << (WIDTH_NODEID + WIDTH_RESTART_COUNTER);

	middle = restartCounter << WIDTH_NODEID;

	right = nodeID;

	nextProposalID = left | middle | right;
	
	return nextProposalID;
}
