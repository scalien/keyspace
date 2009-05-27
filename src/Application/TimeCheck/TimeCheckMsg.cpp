#include "TimeCheckMsg.h"

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

bool TimeCheckMsg::Read(const ByteString& data)
{
	int read;
	
	if (data.length < 1)
		return false;

	switch (data.buffer[0])
	{
		case TIMECHECK_REQUEST:
			read = snreadf(data.buffer, data.length, "%c:%u:%U:%U",
						   &type, &nodeID, &series, &requestTimestamp);
			break;
		case TIMECHECK_RESPONSE:
			read = snreadf(data.buffer, data.length, "%c:%u:%U:%U:%U",
						   &type, &nodeID, &series,
						   &requestTimestamp, &responseTimestamp);
			break;
		default:
			return false;
	}
	
	return (read == (signed)data.length ? true : false);
}

bool TimeCheckMsg::Write(ByteString& data)
{
	int req;
	
	switch (type)
	{
		case TIMECHECK_REQUEST:
			return data.Writef("%c:%u:%U:%U",
							   type, nodeID, series, requestTimestamp);
			break;
		case TIMECHECK_RESPONSE:
			return data.Writef("%c:%u:%U:%U:%U",
							   type, nodeID, series,
							   requestTimestamp, responseTimestamp);
			break;
		default:
			return false;
	}
	
	if (req < 0 || (unsigned)req > data.size)
		return false;
		
	data.length = req;
	return true;
}
