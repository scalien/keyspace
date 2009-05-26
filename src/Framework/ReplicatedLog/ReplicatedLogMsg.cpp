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
	
#define CheckOverflow()		if ((pos - data.buffer) >= (int) data.length || pos < data.buffer) return false;
#define ReadUint64(num)		(num) = strntouint64(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadSeparator()		if (*pos != ':') return false; pos++;
#define ValidateLength()	if ((pos - data.buffer) != (int) data.length) return false;

	pos = data.buffer;
	CheckOverflow();
	ReadUint64(nodeID); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadUint64(restartCounter); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadUint64(leaseEpoch); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadUint64(length); CheckOverflow();
	ReadSeparator();
		
	if (pos - data.buffer != (int) (data.length - length))
		return false;
	
	value.Set(ByteString(data.size - (pos - data.buffer), length, pos));

	return true;
}

bool ReplicatedLogMsg::Write(ByteString& data)
{
	int required;
	
	required = snwritef(data.buffer, data.size, "%d:%U:%U:%M",
			nodeID, restartCounter, leaseEpoch,
			value.length, value.buffer);
	
	if (required < 0 || (unsigned)required > data.size)
		return false;
	
	data.length = required;
	return true;
}
