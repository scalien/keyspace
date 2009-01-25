#ifndef PAXOSSTATE_H
#define PAXOSSTATE_H

#include <math.h>
#include "PaxosConfig.h"
#include "System/Buffer.h"

class PreparerState
{
public:
	bool					active;
	long					n;
	
	long					n_highest_received;
	ByteArray<VALUE_SIZE>	value;
	int						numOpen;
};

class ProposerState
{
public:
	bool					active;
	long					n;
	ByteArray<VALUE_SIZE>	value;
};

class AcceptorState
{
public:
	bool					accepted;
	long					n;
	ByteArray<VALUE_SIZE>	value;
};

class LearnerState
{
public:
	bool					learned;
	ByteArray<VALUE_SIZE>	value;
};

#endif