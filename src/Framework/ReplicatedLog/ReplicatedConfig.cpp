#include "ReplicatedConfig.h"
#include "System/Config.h"
#include "Framework/Database/Table.h"
#include "Framework/Database/Transaction.h"

ReplicatedConfig config;

ReplicatedConfig* ReplicatedConfig::Get()
{
	return &config;
}

bool ReplicatedConfig::Init()
{
	Endpoint	endpoint;

	nodeID = Config::GetIntValue("paxos.nodeID", 0);
	numNodes = Config::GetListNum("paxos.endpoints");
	
	if (numNodes == 0)
	{
		endpoint.Set("0.0.0.0:10000");
		endpoints[0] = endpoint;
		nodeID = 0;
	}
	else
	{
		if (nodeID < 0 || nodeID >= numNodes)
			STOP_FAIL("Configuration error, "
					   "check your paxos.nodeID and paxos.endpoints entry" ,0);

		for (unsigned i = 0; i < numNodes; i++)
		{
			endpoint.Set(Config::GetListValue("paxos.endpoints", i, NULL));
			endpoints[i] = endpoint;
		}
	}
	
	if (numNodes > 1)
		InitRestartCounter();
	
	return true;
}

void ReplicatedConfig::InitRestartCounter()
{
	Log_Trace();
	
	bool			ret;
	unsigned		nread = 0;
	ByteArray<32>	buf;
	Table*			table;
	
	table = database.GetTable("keyspace");
	if (table == NULL)
	{
		restartCounter = 0;
		return;
	}
	
	Transaction tx(table);
	tx.Begin();
	
	ret = table->Get(&tx, "@@restartCounter", buf);

	if (ret)
	{
		restartCounter = strntouint64(buf.buffer, buf.length, &nread);
		if (nread != (unsigned) buf.length)
			restartCounter = 0;
	}
	else
		restartCounter = 0;
	
	restartCounter++;
	
	buf.length =
		snwritef(buf.buffer, buf.size, "%U", restartCounter);
	
	table->Set(&tx, "@@restartCounter", buf);
	tx.Commit();
	
	Log_Trace("Running with restartCounter = %" PRIu64 "", restartCounter);
}

unsigned ReplicatedConfig::MinMajority()
{
	return (unsigned)(floor((double)numNodes / 2) + 1);
}

uint64_t ReplicatedConfig::NextHighest(uint64_t proposalID)
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

unsigned ReplicatedConfig::GetPort()
{
	return endpoints[nodeID].GetPort();
}

unsigned ReplicatedConfig::GetNodeID()
{
	return nodeID;
}

unsigned ReplicatedConfig::GetNumNodes()
{
	return numNodes;
}

uint64_t ReplicatedConfig::GetRestartCounter()
{
	return restartCounter;
}

const Endpoint ReplicatedConfig::GetEndpoint(unsigned i)
{
	if (i < 0 || i >= numNodes)
		ASSERT_FAIL();
	
	return endpoints[i];
}
