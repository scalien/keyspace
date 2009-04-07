#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "System/IO/IOProcessor.h"
#include "Scheduler.h"

#define SLEEP_MSEC 20

class EventLoop : public Scheduler
{
public:	
	static long			RunOnce();	
	static void			Run();
	static void			Shutdown();
};


#endif
