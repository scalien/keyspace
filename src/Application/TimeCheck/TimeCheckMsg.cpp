#include "TimeCheckMsg.h"
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include "System/Common.h"

void TimeCheckMsg::Request(uint64_t series_, uint64_t requestTimestamp_)
{
	type = TIMECHECK_REQUEST;
	series = series_;
	requestTimestamp = requestTimestamp_;
}

void TimeCheckMsg::Response(uint64_t series_, uint64_t requestTimestamp_, uint64_t responseTimestamp_)
{
	type = TIMECHECK_RESPONSE;
	series = series_;
	requestTimestamp = requestTimestamp_;
	responseTimestamp = responseTimestamp_;
}

bool TimeCheckMsg::Read(ByteString& data)
{
	char		type;
	
	unsigned	nread;
	char		*pos;
	
	/*	format for paxos messages, numbers are textual:
		<paxosID>#<type>#<<<type specific>>>
	*/
	
#define CheckOverflow()		if ((pos - data.buffer) >= (int) data.length || pos < data.buffer) return false;
#define ReadUint64_t(num)		(num) = strntouint64_t(pos, data.length - (pos - data.buffer), &nread); \
								if (nread < 1) return false; pos += nread;
#define ReadChar(c)			(c) = *pos; pos++;
#define ReadSeparator()		if (*pos != '#') return false; pos++;
#define ValidateLength()	if ((pos - data.buffer) != (int) data.length) return false;

	pos = data.buffer;
	CheckOverflow();
	ReadChar(type); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadUint64_t(series); CheckOverflow();
	ReadSeparator(); CheckOverflow();
	ReadUint64_t(requestTimestamp);
		
	if (type == TIMECHECK_REQUEST)
	{
		ValidateLength();
		Request(series, requestTimestamp);
		return true;
	}
	else if (type == TIMECHECK_RESPONSE)
	{
		CheckOverflow();
		ReadSeparator(); CheckOverflow();
		ReadUint64_t(responseTimestamp);
		ValidateLength();
		Response(series, requestTimestamp, responseTimestamp);
		return true;
	}
	
	return false;
}

bool TimeCheckMsg::Write(ByteString& data)
{
	unsigned required;
	
	if (type == TIMECHECK_REQUEST)
		required = snprintf(data.buffer, data.size, "%c:%" PRIu64 ":%" PRIu64 "", type,
							series, requestTimestamp);
	else if (type == TIMECHECK_RESPONSE)
		required = snprintf(data.buffer, data.size, "%c:%" PRIu64 ":%" PRIu64 ":%" PRIu64 "", type,
							series, requestTimestamp, responseTimestamp);
	else
		return false;
	
	if (required > data.size)
		return false;
		
	data.length = required;
	return true;
}
