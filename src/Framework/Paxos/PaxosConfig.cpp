#include "PaxosConfig.h"
#include <stdlib.h>
#include <math.h>

bool PaxosConfig::Read(char *filename)
{
	FILE*		f;
	char		buffer[1024];
	Endpoint	endpoint;

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
	
	endpoint.Init(buffer);
	port = endpoint.Port();
	
	// read empty line
	ReadLine();
	
	// now read other nodes, including myself
	while (fgets(buffer, sizeof(buffer), f) != NULL)
	{
		endpoint.Init(buffer);
		endpoints.Add(endpoint);
	}
	
	numNodes = endpoints.size;
	
	return true;
}

int PaxosConfig::MinMajority()
{
	return (floor(numNodes / 2) + 1);
}

long PaxosConfig::NextHighest(long highest_promised)
{
	long round, n;
	
	round = floor(highest_promised / numNodes);
	round++;
	n = round * numNodes + nodeID;
	
	return n;
}
