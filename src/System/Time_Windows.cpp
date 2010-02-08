#ifdef PLATFORM_WINDOWS

#include "Platform.h"
#include <windows.h>

/*
static int gettimeofday (struct timeval *tv, void* tz)
{
	DYNAMIC_TIME_ZONE_INFORMATION	dtzInfo;
	DWORD							tzId;
	LONG							bias;
	union {
		LONGLONG ns100; //time since 1 Jan 1601 in 100ns units
		FILETIME ft;
	} now;

	bias = 0;
	tzId = GetDynamicTimeZoneInformation(&dtzInfo);
	if (tzId == TIME_ZONE_ID_STANDARD)
		bias = dtzInfo.Bias + dtzInfo.StandardBias;
	else if (tzId = TIME_ZONE_ID_DAYLIGHT)
		bias = dtzInfo.Bias + dtzInfo.DaylightBias;

	GetSystemTimeAsFileTime (&now.ft);
	tv->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL);
	tv->tv_sec = (long) ((now.ns100 - 116444736000000000LL) / 10000000LL - bias * 60);
	return (0);
} 
*/

uint64_t Now()
{
	return GetMilliTimestamp();
}

uint64_t NowMicro()
{
	return GetMicroTimestamp();
}

void MSleep(unsigned long msec)
{
	Sleep(msec);
}

#endif
