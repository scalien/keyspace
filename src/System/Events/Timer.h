#ifndef TIMER_H
#define TIMER_H

#include "Callable.h"
#include "System/Time.h"

class Timer
{
public:
    long long		when;
	int				delay;
    Callable*		callable;
    
	Timer()
	{
		when = 0;
		delay = 0;
		callable = NULL;
	}
	
	Timer(long delay_, Callable* callable_)
	{
		delay = delay_;
		callable = callable_;
	}
	
	~Timer() { }
	
	void Set()
	{
		when = Now() + delay;
	}
	
	void Execute()
	{
		Call(callable);
	}
};

inline bool LessThan(Timer* a, Timer* b)
{
    return (a->when < b->when);
}

#endif
