#ifndef HUBSIM_H
#define HUBSIM_H

#include "Link.h"

#define MAX_NODES 10
#define MAX_LINKS ((MAX_NODES * (MAX_NODES - 1)) / 2)

class Node;

class HubSim
{		
public:
	Node*	nodes[MAX_NODES];
	int		numNodes;
	Link*	links[MAX_LINKS];
	int		numLinks;
	Link*	goodLink;
	PacketLossLink*	packetLossLink;
	ReorderingLink*	reorderingLink;
	
	HubSim();

	void AddLink(Link* link);	
	void CreateNode(char *ip, int port);
	Node* GetNode(Endpoint &ep);
	Link* GetLink(Node* n1, Node* n2);
};


#endif
