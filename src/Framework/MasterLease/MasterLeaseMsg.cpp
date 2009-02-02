#include "MasterLeaseMsg.h"
#include "System/Common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void MasterLeaseMsg::ExtendLease(int nodeID_, int epochID_)
{
	type = EXTEND_LEASE;
	nodeID = nodeID_;
	epochID = epochID_;
}

void MasterLeaseMsg::YieldLease(int nodeID_, int epochID_)
{
	type = YIELD_LEASE;
	nodeID = nodeID_;
	epochID = epochID_;
}

bool MasterLeaseMsg::Read(ByteString data, MasterLeaseMsg* msg)
{
#define CheckOverflow()		if ((pos - data.buffer) >= data.length) return false;
#define ReadNumber(num)		(num) = strntol(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadChar(c)			(c) = *pos; pos++;
#define ReadSeparator()		if (*pos != ':') return false; pos++;
#define ValidateLength()	if (pos - data.buffer != data.length) return false;

	char			protocol, type;
	int				nodeID, epochID, nread;
	char*			pos;

	pos = data.buffer;
	CheckOverflow();
	ReadChar(protocol); CheckOverflow();
	ReadSeparator() CheckOverflow();
	ReadChar(type);
	
	if (protocol != PROTOCOL_MASTERLEASE)
		return false;
	
	if (type == EXTEND_LEASE)
	{
		CheckOverflow();
		ReadSeparator() CheckOverflow();
		ReadNumber(nodeID); CheckOverflow();
		ReadSeparator() CheckOverflow();
		ReadNumber(epochID);
		ValidateLength();
	
		msg->ExtendLease(nodeID, epochID);
		return true;
	}
	else if (type == YIELD_LEASE)
	{
		CheckOverflow();
		ReadSeparator() CheckOverflow();
		ReadNumber(nodeID); CheckOverflow();
		ReadSeparator() CheckOverflow();
		ReadNumber(epochID);
		ValidateLength();
		
		msg->YieldLease(nodeID, epochID);
		return true;
	}
	else
		return false;
	
	return true;
}

bool MasterLeaseMsg::Write(MasterLeaseMsg* msg, ByteString& data)
{
	int required;
	
	if (msg->type == EXTEND_LEASE)
	{
		required  = snprintf(data.buffer, data.size, "%c:%c:%d:%d",
			PROTOCOL_MASTERLEASE, msg->type, msg->nodeID, msg->epochID);
	}
	else if(msg->type == YIELD_LEASE)
	{
		required  = snprintf(data.buffer, data.size, "%c:%c:%d:%d",
			PROTOCOL_MASTERLEASE, msg->type, msg->nodeID, msg->epochID);
	}
	else
		return false;
	
	if (required > data.size)
		return false;

	data.length = required;
	return true;
}
