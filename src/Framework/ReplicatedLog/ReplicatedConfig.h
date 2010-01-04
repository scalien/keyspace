#ifndef REPLICATEDCONFIG_H
#define REPLICATEDCONFIG_H

#include "System/Platform.h"
#include "System/IO/Endpoint.h"

#define MAX_CELL_SIZE				256
#define WIDTH_NODEID				8
#define WIDTH_RESTART_COUNTER		16

#define RCONF (ReplicatedConfig::Get())


class ReplicatedConfig
{
public:
	static ReplicatedConfig* Get();
	
	bool			Init();
	
	unsigned		MinMajority();
	uint64_t		NextHighest(uint64_t proposalID);
	unsigned		GetPort();
	unsigned		GetNodeID();
	unsigned		GetNumNodes();
	uint64_t		GetRestartCounter();
	const Endpoint	GetEndpoint(unsigned i);

private:
	unsigned		nodeID;
	unsigned		numNodes; // same as endpoints.size
	uint64_t		restartCounter;	
	Endpoint		endpoints[MAX_CELL_SIZE];

private:
	void			InitRestartCounter();
};

#endif
