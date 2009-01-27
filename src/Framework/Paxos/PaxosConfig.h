#ifndef PAXOSCONFIG_H
#define PAXOSCONFIG_H

#include "System/Containers/List.h"
#include "System/IO/Endpoint.h"

class PaxosConfig
{
public:
	int				nodeID;
	int				numNodes; // same as endpoints.size
	
	int				port;

	List<Endpoint>	endpoints;

	bool			Read(char* filename);
	
	int				MinMajority();
	
	long			NextHighest(long highest_promised);
};

#endif
