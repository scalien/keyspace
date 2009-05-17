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
	onSendTimeout(this, &TimeCheck::OnSendTimeout),
	seriesTimeout(SERIES_TIMEOUT, &onSeriesTimeout),
	sendTimeout(SERIES_TIMEOUT/(2*NUMMSGS), &onSendTimeout)
{
}

void TimeCheck::Init()
{
	numReplies = new int[ReplicatedConfig::Get()->numNodes];
	totalSkew = new double[ReplicatedConfig::Get()->numNodes];
	
	InitTransport();
	
	series = 0;
	
	NextSeries();
}

void TimeCheck::InitTransport()
{
	unsigned	i;
	Endpoint	endpoint;
	
	reader = new TransportUDPReader;
	if (!reader->Init(ReplicatedConfig::Get()->GetPort() + TIMECHECK_PORT_OFFSET))
		STOP_FAIL("Cannot initialize TimeCheck", 1);
	reader->SetOnRead(&onRead);
	
	writers = (TransportWriter**) Alloc(sizeof(TransportWriter*) * ReplicatedConfig::Get()->numNodes);
	for (i = 0; i < ReplicatedConfig::Get()->numNodes; i++)
	{
		endpoint = ReplicatedConfig::Get()->endpoints[i];
		endpoint.SetPort(endpoint.GetPort() + TIMECHECK_PORT_OFFSET);
		writers[i] = new TransportUDPWriter;
		if (!writers[i]->Init(endpoint))
			STOP_FAIL("Cannot initialize TimeCheck", 1);
	}
}

void TimeCheck::NextSeries()
{
	unsigned i;

	series++;
	sentInSeries = 0;
	
	for (i = 0; i < ReplicatedConfig::Get()->numNodes; i++)
	{
		numReplies[i] = 0;
		totalSkew[i] = 0.0;
	}

	EventLoop::Reset(&sendTimeout);

	EventLoop::Reset(&seriesTimeout);
}

void TimeCheck::OnSeriesTimeout()
{
	unsigned i;
	
	for (i = 0; i < ReplicatedConfig::Get()->numNodes; i++)
	{
		if (numReplies[i] > 0)
		{
			double skew = totalSkew[i] / numReplies[i];
			
//			Log_Message("%lf %d\n", totalSkew[i], numReplies[i]);
//			Log_Message("skew for nodeID %d: %lf\n", i, skew);
			
			if (skew > MAX_CLOCK_SKEW)
				STOP_FAIL("Clock skew between nodes exceeds allowed maximum", 1);
		}
	}
	
	NextSeries();
}

void TimeCheck::OnSendTimeout()
{
	unsigned i;
	
	for (i = 0; i < ReplicatedConfig::Get()->numNodes; i++)
	{
		msg.Request(ReplicatedConfig::Get()->nodeID, series, Now());
		msg.Write(data);
		writers[i]->Write(data);
	}
	
	sentInSeries++;
	
	if (sentInSeries < NUMMSGS)
		EventLoop::Reset(&sendTimeout);
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
		msg.Response(ReplicatedConfig::Get()->nodeID, msg.series, msg.requestTimestamp, Now());
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
		if (msg.nodeID < ReplicatedConfig::Get()->numNodes)
		{
			numReplies[msg.nodeID]++;
			totalSkew[msg.nodeID] += skew;
			
//			double skew = totalSkew[msg.nodeID] / numReplies[msg.nodeID];
//			Log_Message("Running skew for nodeID %d after %d msgs: %lf\n",
//				msg.nodeID, numReplies[msg.nodeID], skew);
		}
	}
}
