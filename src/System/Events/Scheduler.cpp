#include "Scheduler.h"

void Scheduler::Add(Timer* timer)
{
	timer->OnAdd();
	timer->active = true;
	timers.Add(timer);
}

void Scheduler::Remove(Timer* timer)
{
	timers.Remove(timer);
	timer->active = false;
}

void Scheduler::Reset(Timer* timer)
{
	Remove(timer);
	Add(timer);
}
