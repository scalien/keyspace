#ifndef REPLICATEDLOGMSG_H
#define REPLICATEDLOGMSG_H

#include "System/Buffer.h"

#define BS_MSG_NOP	ByteString(strlen("NOP"), strlen("NOP"), "NOP")

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
