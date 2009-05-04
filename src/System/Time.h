#ifndef TIME_H
#define TIME_H

#include <time.h>
#include <sys/timeb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

inline uint64_t Now()
{
	uint64_t now;
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
