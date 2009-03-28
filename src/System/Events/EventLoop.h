#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "System/IO/IOProcessor.h"
#include "System/Time.h"
#include "Scheduler.h"

#define SLEEP_MSEC 20

class EventLoop : public Scheduler
{
public:
	static EventLoop*	Get();
	
	long				RunOnce();	
	void				Run();
	void				Shutdown();
};


#endif
