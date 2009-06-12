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

	virtual ~Timer() {}
		
	Timer(Callable* callable_)
	{
		when = 0;
		callable = callable_;
		active = false;
	}
		
	void Set(uint64_t when_)
	{
		when = when_;
	}
	
	bool IsActive()
	{
		return active;
	}
	
	uint64_t When()
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
	uint64_t		when;
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
	
	CdownTimer(uint64_t delay_, Callable* callable_) : Timer(callable_)
	{
		delay = delay_;
	}
		
	void SetDelay(uint64_t delay_)
	{
		delay = delay_;
	}

	virtual void OnAdd()
	{
		when = Now() + delay;
	}
	
private:
	uint64_t delay;
};

inline bool LessThan(Timer* a, Timer* b)
{
    return (a->When() < b->When());
}

#endif
