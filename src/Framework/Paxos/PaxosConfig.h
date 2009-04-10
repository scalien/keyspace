#ifndef PAXOSCONFIG_H
#define PAXOSCONFIG_H

#include <stdint.h>
#include "System/IO/Endpoint.h"

#define MAX_CELL_SIZE 256

class PaxosConfig
{
public:
	static PaxosConfig* Get();
	
	bool			Init(char* filename);
	
	int				MinMajority();
	uint64_t			NextHighest(uint64_t proposalID);

	int				nodeID;
	int				numNodes; // same as endpoints.size
	uint64_t			restartCounter;	
	int				port;
	Endpoint		endpoints[MAX_CELL_SIZE];

private:
	void			InitRestartCounter();
};

#endif
