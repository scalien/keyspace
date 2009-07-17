#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "Time.h"

class Stopwatch
{
public:
	long elapsed;
	
	Stopwatch () { Reset(); }
	
	void Reset() { elapsed = 0; }
	void Start() { started = Now(); }
	void Stop() { elapsed += Now() - started; }

private:
	long started;
};

#endif
