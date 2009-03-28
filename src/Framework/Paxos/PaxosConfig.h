#ifndef PAXOSCONFIG_H
#define PAXOSCONFIG_H

#include "System/Types.h"
#include "System/IO/Endpoint.h"

#define MAX_CELL_SIZE 256

class PaxosConfig
{
public:
	static PaxosConfig* Get();
	
	bool			Init(char* filename);
	
	int				MinMajority();
	ulong64			NextHighest(ulong64 proposalID);

	int				nodeID;
	int				numNodes; // same as endpoints.size
	ulong64			restartCounter;	
	int				port;
	Endpoint		endpoints[MAX_CELL_SIZE];

private:
	void			InitRestartCounter();
};

#endif
