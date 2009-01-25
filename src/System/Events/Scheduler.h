#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "System/Containers/SortedList.h"
#include "Timer.h"

class Scheduler
{
public:
	SortedList<Timer*>	timers;
		
	void Add(Timer* timer);
	
	void Remove(Timer* timer);
	
	void Reset(Timer* timer);
	
	void Shutdown()					{ /* todo: deallocate timers? */ }
};

#endif