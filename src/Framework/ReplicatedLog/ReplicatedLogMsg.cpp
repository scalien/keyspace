#include "ReplicatedLogMsg.h"

bool ReplicatedLogMsg::Init(unsigned nodeID_, uint64_t restartCounter_,
	uint64_t leaseEpoch_, ByteString& value_)
{
	nodeID = nodeID_;
	restartCounter = restartCounter_;
	leaseEpoch = leaseEpoch_;
	value = value_;
		
	return true;
}

bool ReplicatedLogMsg::Read(const ByteString& data)
{
	int read;
	
	read = snreadf(data.buffer, data.size, "%u:%U:%U:%N",
				   &nodeID, &restartCounter, &leaseEpoch, &value);
	
	return (read == (signed)data.length ? true : false);
}

bool ReplicatedLogMsg::Write(ByteString& data)
{
	return data.Writef("%u:%U:%U:%M",
					   nodeID, restartCounter, leaseEpoch, &value);
}
