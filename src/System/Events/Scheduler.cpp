#include "Scheduler.h"

void Scheduler::Add(Timer* timer)
{
	timer->Set();
	timers.Add(timer);
}

void Scheduler::Remove(Timer* timer)
{
	timers.Remove(timer);
}

void Scheduler::Reset(Timer* timer)
{
	Remove(timer);
	Add(timer);
}
