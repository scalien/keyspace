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
	bool ret;
	
	if (data.length < 1)
		return false;

	switch (data.buffer[0])
	{
		case TIMECHECK_REQUEST:
			ret = snreadf(data.buffer, data.length, "%c:%u:%U:%U", &type, &nodeID,
						  &series, &requestTimestamp);
			break;
		case TIMECHECK_RESPONSE:
			ret = snreadf(data.buffer, data.length, "%c:%u:%U:%U:%U", &type, &nodeID,
						  &series, &requestTimestamp, &responseTimestamp);
			break;
		default:
			ret = false;
	}
	
	return ret;
}

bool TimeCheckMsg::Write(ByteString& data)
{
	unsigned req;
	
	switch (type)
	{
		case TIMECHECK_REQUEST:
			req = snwritef(data.buffer, data.size, "%c:%u:%U:%U", type, nodeID,
						   series, requestTimestamp);
			break;
		case TIMECHECK_RESPONSE:
			req = snwritef(data.buffer, data.size, "%c:%u:%U:%U:%U", type,
						   nodeID, series, requestTimestamp, responseTimestamp);
			break;
		default:
			return false;
	}
	
	if (req < 0 || (unsigned)req > data.size)
		return false;
		
	data.length = req;
	return true;
}
