#include "ReplicatedLogMsg.h"
#include <stdio.h>

bool ReplicatedLogMsg::Init(unsigned nodeID_, ulong64 restartCounter_, ulong64 leaseEpoch_,
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
	int			nread, length;
	char		*pos;
	
#define CheckOverflow()		if ((pos - data.buffer) >= data.length) return false;
#define ReadNumber(num)		(num) = strntoulong64(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadSeparator()		if (*pos != '|') return false; pos++;
#define ValidateLength()	if ((pos - data.buffer) != data.length) return false;

	pos = data.buffer;
	CheckOverflow();
	ReadNumber(nodeID); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadNumber(restartCounter); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadNumber(leaseEpoch); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadNumber(length); CheckOverflow();
	ReadSeparator();
		
	if (pos - data.buffer != data.length - length)
		return false;
	
	value.Set(ByteString(data.size - (pos - data.buffer), length, pos));

	return true;
}

bool ReplicatedLogMsg::Write(ByteString& data)
{
	int required;
	
	required = snprintf(data.buffer, data.size, "%d|%llu|%llu|%d|%.*s",
			nodeID, restartCounter, leaseEpoch,
			value.length, value.length, value.buffer);
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;
}
