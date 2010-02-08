#ifdef PLATFORM_WINDOWS

#include "Platform.h"
#include <windows.h>

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
