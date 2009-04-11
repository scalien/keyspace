#include "ReplicatedLogMsg.h"
#include <stdio.h>
#include <inttypes.h>

bool ReplicatedLogMsg::Init(unsigned nodeID_, uint64_t restartCounter_, uint64_t leaseEpoch_,
	ByteString& value_)
{
	nodeID = nodeID_;
	restartCounter = restartCounter_;
	leaseEpoch = leaseEpoch_;
	if (!value.Set(value_))
		return false;
		
	return true;
}

bool ReplicatedLogMsg::Read(ByteString& data)
{
	unsigned	nread, length;
	char		*pos;
	
#define CheckOverflow()		if ((pos - data.buffer) >= data.length) return false;
#define ReadUint64_t(num)		(num) = strntouint64_t(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadSeparator()		if (*pos != '|') return false; pos++;
#define ValidateLength()	if ((pos - data.buffer) != data.length) return false;

	pos = data.buffer;
	CheckOverflow();
	ReadUint64_t(nodeID); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadUint64_t(restartCounter); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadUint64_t(leaseEpoch); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadUint64_t(length); CheckOverflow();
	ReadSeparator();
		
	if (pos - data.buffer != data.length - length)
		return false;
	
	value.Set(ByteString(data.size - (pos - data.buffer), length, pos));

	return true;
}

bool ReplicatedLogMsg::Write(ByteString& data)
{
	int required;
	
	required = snprintf(data.buffer, data.size, "%d|%" PRIu64 "|%" PRIu64 "|%d|%.*s",
			nodeID, restartCounter, leaseEpoch,
			value.length, value.length, value.buffer);
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;
}
