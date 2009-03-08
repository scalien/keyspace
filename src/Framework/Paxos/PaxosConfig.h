#ifndef PAXOSCONFIG_H
#define PAXOSCONFIG_H

#include "System/Types.h"
#include "System/Containers/List.h"
#include "System/IO/Endpoint.h"

class PaxosConfig
{
public:
	int				nodeID;
	int				numNodes; // same as endpoints.size
	
	int				port;

	List<Endpoint>	endpoints;

	bool			Init(char* filename);
	
	int				MinMajority();
	
	ulong64			NextHighest(ulong64 proposalID);

private:
	void			InitRestartCounter();
	
	ulong64			restartCounter;
};

#endif
