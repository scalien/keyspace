#ifndef REPLICATEDLOGMSG_H
#define REPLICATEDLOGMSG_H

#include <unistd.h>
#include <string.h>
#include "System/Buffer.h"
#include "System/Types.h"
#include "Framework/Paxos/PaxosConsts.h"

class ReplicatedLogMsg
{
public:
	unsigned					nodeID;
	ulong64						restartCounter;
	ulong64						leaseEpoch;
	ByteArray<PAXOS_BUFSIZE>	value;
	
	bool						Init(unsigned nodeID_, ulong64 restartCounter_, ulong64 leaseEpoch_,
									ByteString& value_);
		
	bool						Read(ByteString& data);
	bool						Write(ByteString& data);
};

#endif
