#ifndef MEMLOG_H
#define MEMLOG_H

#include "System/Containers/List.h"
#include "Framework/Paxos/PaxosMsg.h"

typedef struct
{
	long					paxosID;
	ByteArray<VALUE_SIZE>	value;
} Entry;

class MemLog
{
public:
	List<Entry*>		log;		/* head is the most recent entry! */

	Entry*				Head();

	bool				Push(int paxosID, ByteString value);
	Entry*				Get(int paxosID);
};

#endif