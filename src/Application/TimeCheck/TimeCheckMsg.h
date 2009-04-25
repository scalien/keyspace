#ifndef TIMECHECKMSG_H
#define TIMECHECKMSG_H

#include "System/Buffer.h"
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define TIMECHECK_REQUEST			'1'
#define	TIMECHECK_RESPONSE			'2'

class TimeCheckMsg
{
public:
	char			type;
	uint64_t		series;
	uint64_t		requestTimestamp;
	uint64_t		responseTimestamp;
	
	void			Request(uint64_t series_, uint64_t requestTimestamp_);
	void			Response(uint64_t series_, uint64_t requestTimestamp_, uint64_t responseTimestamp_);
	
	bool			Read(ByteString& data);
	bool			Write(ByteString& data);
};

#endif
