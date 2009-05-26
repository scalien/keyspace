#include "EventLoop.h"

EventLoop eventLoop;

long EventLoop::RunTimers()
{
	Timer** it;
	Timer* timer;
	uint64_t now;
	
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

void EventLoop::RunOnce()
{
	long sleep;

	sleep = RunTimers();
	
	if (sleep < 0)
		sleep = SLEEP_MSEC;
	
	IOProcessor::Poll(sleep);
}

void EventLoop::Run()
{
	while(true)
		RunOnce();
}

void EventLoop::Shutdown()
{
	Scheduler::Shutdown();
}
