#include "TimeCheck.h"
#include "System/Log.h"
#include "System/Config.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

TimeCheck::TimeCheck() :
onRead(this, &TimeCheck::OnRead),
onWrite(this, &TimeCheck::OnWrite)
{
}

bool TimeCheck::Init()
{
	int port;
	
	port = Config::GetIntValue("timecheck.port", -1);
	if (port < 0)
		ASSERT_FAIL();
	
	socket.Create(UDP);
	socket.Bind(port);
	socket.SetNonblocking();
	Log_Message("Bound to socket %d", socket.fd);
	
	udpread.fd = socket.fd;
	udpread.data = data;
	udpread.onComplete = &onRead;
	
	udpwrite.fd = socket.fd;
	udpwrite.data = data;
	udpwrite.onComplete = &onWrite;

	return IOProcessor::Add(&udpread);

	series = 0;
}

void TimeCheck::NextCheck()
{
	

	series++;
}

void TimeCheck::SendRequests(Endpoint &endpoint)
{
	reqdata.length = snprintf(reqdata.buffer, reqdata.size, "g:%d:%" PRIu64 "", series, Now());
	socket.SendTo(reqdata.buffer, reqdata.length, endpoint);
}

void TimeCheck::OnRead()
{
	Log_Trace();
	
	unsigned msgseries, nread;
	
	if (udpread.data.length >= 3)
	{
	}
	else
		IOProcessor::Add(&udpread);
}

void TimeCheck::OnWrite()
{
	Log_Trace();
	
	IOProcessor::Add(&udpread);
}
