#include "EventLoop.h"

EventLoop eventLoop;

EventLoop* EventLoop::Get()
{
	return &eventLoop;
}
