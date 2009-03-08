#ifndef TIME_H
#define TIME_H

#include <sys/timeb.h>
#include <unistd.h>
#include <stdio.h>
#include "Types.h"

inline ulong64 Now()
{
	ulong64 now;
	struct timeb tp;
	
    ftime(&tp);
	
	now = tp.time;
	now *= 1000;
	now += tp.millitm;
    
	return now;
}

inline void Sleep(unsigned long msec)
{
	usleep(msec * 1000);
}

#endif
