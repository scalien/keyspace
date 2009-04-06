#include "EventLoop.h"

EventLoop eventLoop;

long EventLoop::RunOnce()
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

void EventLoop::Run()
{
	long sleep;
	
	while(true)
	{
		sleep = RunOnce();
		
		if (sleep < 0)
			sleep = SLEEP_MSEC;
		
		IOProcessor::Poll(sleep);
	}
}

void EventLoop::Shutdown()
{
	Scheduler::Shutdown();
}
