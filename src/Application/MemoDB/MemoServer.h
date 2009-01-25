#ifndef MEMOSERVER_H
#define MEMOSERVER_H

#include "IOProcessor.h"
#include "Scheduler.h"
#include "Socket.h"
#include "MemoDB.h"

#define MEMOSERVER_PORT	5000

class MemoServer
{
public:
	MemoServer();
	
	IOProcessor*		ioproc;
	Scheduler*			scheduler;
	Socket				listener;
	TCPRead				tcpread;
	
	MemoDB*				memodb;

	bool				Init(IOProcessor* ioproc_, Scheduler* scheduler_, 
							MemoDB* memodb_, int port);
	
	void				OnConnect();

	MFunc<MemoServer>	onConnect;
};

#endif