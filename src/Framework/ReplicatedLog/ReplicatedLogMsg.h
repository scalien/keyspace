#ifndef REPLICATEDLOGMSG_H
#define REPLICATEDLOGMSG_H

#include "System/Buffer.h"

#define MSG_NOP		"NOP"
#define BS_MSG_NOP	ByteString(strlen(MSG_NOP), strlen(MSG_NOP), MSG_NOP)

class ReplicatedLogMsg
{
public:
	unsigned	nodeID;
	uint64_t	restartCounter;
	uint64_t	leaseEpoch;
	ByteString	value;
	
	bool		Init(unsigned nodeID_, uint64_t restartCounter_,
				uint64_t leaseEpoch_, ByteString& value_);
		
	bool		Read(const ByteString& data);
	bool		Write(ByteString& data);
};

#endif
