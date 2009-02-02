#ifndef TIMER_H
#define TIMER_H

#include "Callable.h"
#include "System/Time.h"

class Timer
{
public:
    bool			active;
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
		active = false;
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

template<class T>
class TimerM : public Timer
{
public:
	typedef void (T::*Callback)();
	
	TimerM(long delay, T* object, Callback callback)
	: callable(object, callback), Timer(delay, &callable)
	{}
	
private:
	MFunc<T>	callable;
};

class TimerC : public Timer
{
public:
	typedef void (*Callback)();
	
	TimerC(long delay, Callback callback)
	: callable(callback), Timer(delay, &callable)
	{}

private:
	CFunc		callable;
};

inline bool LessThan(Timer* a, Timer* b)
{
    return (a->when < b->when);
}

#endif
