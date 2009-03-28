#ifndef TIMER_H
#define TIMER_H

#include "System/Time.h"
#include "Callable.h"

class Timer
{
friend class Scheduler;

public:
	Timer()
	{
		when = 0;
		callable = NULL;
		active = false;
	}
		
	Timer(Callable* callable_)
	{
		when = 0;
		callable = callable_;
		active = false;
	}
		
	void Set(ulong64 when_)
	{
		when = when_;
	}
	
	bool IsActive()
	{
		return active;
	}
	
	ulong64 When()
	{
		return when;
	}
	
	void Execute()
	{
		Call(callable);
	}
	
	virtual void OnAdd()
	{
	}

protected:
    bool			active;
	ulong64			when;
    Callable*		callable;
};

class CdownTimer : public Timer
{
public:
	CdownTimer() : Timer()
	{
		delay = 0;
	}
	
	CdownTimer(Callable* callable_) : Timer(callable_)
	{
		delay = 0;
	}
	
	CdownTimer(long delay_, Callable* callable_) : Timer(callable_)
	{
		delay = delay_;
	}
		
	void SetDelay(unsigned delay_)
	{
		delay = delay_;
	}

	virtual void OnAdd()
	{
		when = Now() + delay;
	}
	
private:
	unsigned delay;
};

inline bool LessThan(Timer* a, Timer* b)
{
    return (a->When() < b->When());
}

#endif
