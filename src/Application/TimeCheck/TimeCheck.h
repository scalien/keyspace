#ifndef TIMECHECK_H
#define TIMECHECK_H

#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"

#define TIMECHECK_PORT	4000

class TimeCheck
{
public:
	TimeCheck();

private:
	Socket				socket;
	UDPRead				udpread;
	UDPWrite			udpwrite;
	ByteArray<1024>		data;

	bool				Init();
	
	void				OnRead();
	void				OnWrite();
	
	void				NextCheck();
	void				SendRequests(Endpoint &endpoint);
	
	MFunc<TimeCheck>	onRead;
	MFunc<TimeCheck>	onWrite;
	
	unsigned			series;
	ByteArray<1024>		reqdata;
};

#endif
