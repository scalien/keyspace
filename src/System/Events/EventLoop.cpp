#include "EventLoop.h"

static uint64_t now;

long EventLoop::RunTimers()
{
	Timer** it;
	Timer* timer;
	
	for (it = timers.Head(); it != NULL; it = timers.Head())
	{
		timer = *it;
		
		UpdateTime();
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

uint64_t EventLoop::Now()
{
	if (now == 0)
		UpdateTime();
	
	return now;
}

void EventLoop::UpdateTime()
{
	now = ::Now();
}
