#include "TimeCheckMsg.h"
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include "System/Common.h"

void TimeCheckMsg::Request(unsigned nodeID_, uint64_t series_, uint64_t requestTimestamp_)
{
	type = TIMECHECK_REQUEST;
	nodeID = nodeID_;
	series = series_;
	requestTimestamp = requestTimestamp_;
}

void TimeCheckMsg::Response(unsigned nodeID_, uint64_t series_,
							uint64_t requestTimestamp_, uint64_t responseTimestamp_)
{
	type = TIMECHECK_RESPONSE;
	nodeID = nodeID_;
	series = series_;
	requestTimestamp = requestTimestamp_;
	responseTimestamp = responseTimestamp_;
}

bool TimeCheckMsg::Read(ByteString& data)
{
	if (snreadf(data.buffer, data.length, "%c:%U:%U:%U", &type, &nodeID,
				&series, &requestTimestamp))
	{
		if (type != TIMECHECK_REQUEST)
			return false;
	}
	else if (snreadf(data.buffer, data.length, "%c:%u:%U:%U:%U", &type, &nodeID,
					 &series, &requestTimestamp, &responseTimestamp))
	{
		if (type != TIMECHECK_RESPONSE)
			return false;
	}
	else
		return false;
	
	return true;
}

bool TimeCheckMsg::Write(ByteString& data)
{
	unsigned required;
	
	if (type == TIMECHECK_REQUEST)
		required = snwritef(data.buffer, data.size, "%c:%d:%U:%U", type, nodeID,
							series, requestTimestamp);
	else if (type == TIMECHECK_RESPONSE)
		required = snwritef(data.buffer, data.size, "%c:%u:%U:%U:%U", type,
							nodeID, series, requestTimestamp, responseTimestamp);
	else
		return false;
	
	if (required < 0 || (unsigned)required > data.size)
		return false;
		
	data.length = required;
	return true;
}
