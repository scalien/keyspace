#ifndef REPLICATEDCONFIG_H
#define REPLICATEDCONFIG_H

#include <stdint.h>
#include "System/IO/Endpoint.h"

#define MAX_CELL_SIZE 256

class ReplicatedConfig
{
public:
	static ReplicatedConfig* Get();
	
	bool			Init();
	
	unsigned		MinMajority();
	uint64_t		NextHighest(uint64_t proposalID);
	unsigned		GetPort();

	unsigned		nodeID;
	unsigned		numNodes; // same as endpoints.size
	uint64_t		restartCounter;	
	Endpoint		endpoints[MAX_CELL_SIZE];

private:
	void			InitRestartCounter();
};

#endif
