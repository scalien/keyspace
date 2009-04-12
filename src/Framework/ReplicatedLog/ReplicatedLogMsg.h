#ifndef REPLICATEDLOGMSG_H
#define REPLICATEDLOGMSG_H

#include <unistd.h>
#include <string.h>
#include "System/Buffer.h"
#include <stdint.h>
#include "Framework/Paxos/PaxosConsts.h"

#define MSG_NOP		"NOP"
#define BS_MSG_NOP	ByteString(strlen(MSG_NOP), strlen(MSG_NOP), MSG_NOP)

class ReplicatedLogMsg
{
public:
	unsigned					nodeID;
	uint64_t					restartCounter;
	uint64_t					leaseEpoch;
	ByteArray<PAXOS_BUF_SIZE>	value;
	
	bool						Init(unsigned nodeID_, uint64_t restartCounter_, uint64_t leaseEpoch_,
									ByteString& value_);
		
	bool						Read(ByteString& data);
	bool						Write(ByteString& data);
};

#endif
