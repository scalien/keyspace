#ifndef TIME_H
#define TIME_H

#include <sys/timeb.h>
#include <unistd.h>
#include <stdio.h>

inline long long Now()
{
	long long now;
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
