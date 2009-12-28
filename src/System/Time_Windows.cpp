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

void USleep(unsigned long msec)
{
	Sleep(msec);
}
