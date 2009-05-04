#ifndef TIME_H
#define TIME_H

#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

inline uint64_t Now()
{
	uint64_t now;
	struct timeval tv;
	
    gettimeofday(&tv, NULL);
	
	now = tv.tv_sec;
	now *= 1000;
	now += tv.tv_usec / 1000;
    
	return now;
}

inline void Sleep(unsigned long msec)
{
	usleep(msec * 1000);
}

#endif
