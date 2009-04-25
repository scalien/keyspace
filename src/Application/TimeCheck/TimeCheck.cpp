#include "TimeCheck.h"
#include "System/Log.h"
#include "System/Time.h"
#include "System/Events/EventLoop.h"
#include "Framework/Transport/TransportUDPReader.h"
#include "Framework/Transport/TransportUDPWriter.h"
#include "Framework/PaxosLease/PLeaseConsts.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

TimeCheck::TimeCheck() :
	onRead(this, &TimeCheck::OnRead),
	onSeriesTimeout(this, &TimeCheck::OnSeriesTimeout),
	seriesTimeout(SERIES_TIMEOUT, &onSeriesTimeout)
{
}

void TimeCheck::Init()
{
	numReplies = new int[PaxosConfig::Get()->numNodes];
	totalSkew = new double[PaxosConfig::Get()->numNodes];
	
	InitTransport();
	
	series = 666;
	
	NextSeries();
}

void TimeCheck::InitTransport()
{
	unsigned	i;
	Endpoint	endpoint;
	
	reader = new TransportUDPReader;
	if (!reader->Init(PaxosConfig::Get()->port + TIMECHECK_PORT_OFFSET))
		ASSERT_FAIL();
	reader->SetOnRead(&onRead);
	
	writers = (TransportWriter**) Alloc(sizeof(TransportWriter*) * PaxosConfig::Get()->numNodes);
	for (i = 0; i < PaxosConfig::Get()->numNodes; i++)
	{
		endpoint = PaxosConfig::Get()->endpoints[i];
		endpoint.SetPort(endpoint.GetPort() + TIMECHECK_PORT_OFFSET);
		writers[i] = new TransportUDPWriter;
		if (!writers[i]->Init(endpoint))
			ASSERT_FAIL();
	}
}

void TimeCheck::NextSeries()
{
	unsigned i, j;

	series++;
	
	for (i = 0; i < PaxosConfig::Get()->numNodes; i++)
	{
		for (j = 0; j < NUMMSGS; j++)
		{
			msg.Request(PaxosConfig::Get()->nodeID, series, Now());
			msg.Write(data);
			writers[i]->Write(data);
		}
		
		numReplies[i] = 0;
		totalSkew[i] = 0.0;
	}

	EventLoop::Reset(&seriesTimeout);
}

void TimeCheck::OnSeriesTimeout()
{
	unsigned i;
	
	for (i = 0; i < PaxosConfig::Get()->numNodes; i++)
	{
		if (numReplies[i] > 0)
		{
			double skew = totalSkew[i] / numReplies[i];
			
			Log_Message("%lf %d\n", totalSkew[i], numReplies[i]);
			Log_Message("skew for nodeID %d: %lf\n", i, skew);
			
			if (skew > MAX_CLOCK_SKEW)
				STOP_FAIL("Clock skew between nodes exceeds allowed maximum", 1);
		}
	}
	
	NextSeries();
}

void TimeCheck::OnRead()
{
	ByteString bs;
	reader->GetMessage(bs);
	if (!msg.Read(bs))
		ASSERT_FAIL();

	if (msg.type == TIMECHECK_REQUEST)
	{
		unsigned senderID = msg.nodeID;
		//Log_Message("%" PRIu64 " %" PRIu64 "", msg.series, msg.requestTimestamp);
		msg.Response(PaxosConfig::Get()->nodeID, msg.series, msg.requestTimestamp, Now());
		msg.Write(data);
		writers[senderID]->Write(data);
	}
	else if (msg.type == TIMECHECK_RESPONSE)
	{
		if (msg.series != series)
			return;
		uint64_t now = Now();
		if (now < msg.requestTimestamp)
			ASSERT_FAIL();
		uint64_t elapsed = now - msg.requestTimestamp;
		double middle = msg.requestTimestamp + elapsed/2.0;
		double skew = msg.responseTimestamp - middle;
		//Log_Message("%" PRIu64 "", now);
		//Log_Message("%lf", skew);
		if (msg.nodeID < PaxosConfig::Get()->numNodes)
		{
			//Log_Message("%d", msg.nodeID);
			numReplies[msg.nodeID]++;
			totalSkew[msg.nodeID] += skew;
		}
	}
}
