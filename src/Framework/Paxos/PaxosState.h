#ifndef PAXOSSTATE_H
#define PAXOSSTATE_H

#include <math.h>
#include "System/Buffer.h"
#include "System/Types.h"

class ProposerState
{
public:
	bool					preparing;
	bool					proposing;
	
	ulong64					n_proposing;
	ulong64					n_highest_received;
	int						numOpen;
	ByteArray<VALUE_SIZE>	value;
	
	// multi paxos
	bool					leader;
	
	bool Active() { return (preparing || proposing); }
	
	bool Valid() { return !(preparing && proposing); }
	
	void Reset()
	{
		preparing = false;
		proposing = false;
		n_proposing = 0;
		n_highest_received = 0;
		numOpen = 0;
		value.Clear();
	}	
};

class AcceptorState
{
public:
	bool					accepted;
	bool					learned;
	
	ulong64					n_highest_promised;
	
	ulong64					n_accepted;
	ByteArray<VALUE_SIZE>	value;
	
	void Reset()
	{
		accepted = false;
		learned = false;
		n_highest_promised = 0;
		value.Clear();
	}
};

#endif