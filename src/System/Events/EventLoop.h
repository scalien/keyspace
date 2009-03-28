#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "System/IO/IOProcessor.h"
#include "System/Time.h"
#include "Scheduler.h"

#define SLEEP_MSEC 20

class EventLoop : public Scheduler
{
public:
	IOProcessor*					ioproc;
	
	static EventLoop* Get();
	
	EventLoop()						{ ioproc = NULL; }

	void Init()
	{
		ioproc = IOProcessor::Get();
	}
			
	long RunOnce()
	{
		Timer** it;
		Timer* timer;
		ulong64 now;
		
		for (it = timers.Head(); it != NULL; it = timers.Head())
		{
			timer = *it;
			now = Now();
			
			if (timer->When() <= now)
			{
				Remove(timer);
				timer->Execute();
			}
			else
				return timer->When() - now;
		}
		
		return -1; // no timers to wait for
	}
	
	void Run()
	{
		long sleep;
		
		while(true)
		{
			sleep = RunOnce();
			
			if (sleep < 0)
				sleep = SLEEP_MSEC;
			
			if (ioproc)
				ioproc->Poll(sleep);
			else
				Sleep(sleep);
		}
	}
	
	void Shutdown()
	{
		Scheduler::Shutdown();
	}
};


#endif
