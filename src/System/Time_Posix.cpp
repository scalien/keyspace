#ifndef PLATFORM_WINDOWS

#include "Time.h"

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
	usleep(msec * 1000);
}

#endif
