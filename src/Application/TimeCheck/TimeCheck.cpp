#include "TimeCheck.h"
#include "System/Log.h"
#include "System/Time.h"
#include "System/Events/EventLoop.h"
#include "Framework/Transport/TransportUDPReader.h"
#include "Framework/Transport/TransportUDPWriter.h"
#include "Framework/PaxosLease/PLeaseConsts.h"

TimeCheck::TimeCheck() :
	onRead(this, &TimeCheck::OnRead),
	onSeriesTimeout(this, &TimeCheck::OnSeriesTimeout),
	onSendTimeout(this, &TimeCheck::OnSendTimeout),
	seriesTimeout(SERIES_TIMEOUT, &onSeriesTimeout),
	sendTimeout(SERIES_TIMEOUT/(2*NUMMSGS), &onSendTimeout)
{
}

void TimeCheck::Init(bool verbosity_, bool failOnSkew_)
{
	verbosity = verbosity_;
	failOnSkew = failOnSkew_;
	numReplies = new int[RCONF->GetNumNodes()];
	totalSkew = new double[RCONF->GetNumNodes()];
	
	InitTransport();
	
	series = 0;
	
	NextSeries();
}

void TimeCheck::InitTransport()
{
	unsigned	i;
	Endpoint	endpoint;
	
	reader = new TransportUDPReader;
	if (!reader->Init(RCONF->GetPort() + TIMECHECK_PORT_OFFSET))
		STOP_FAIL("Cannot initialize TimeCheck", 1);
	reader->SetOnRead(&onRead);
	
	writers = (TransportUDPWriter**)
			  Alloc(sizeof(TransportUDPWriter*) * RCONF->GetNumNodes());
	for (i = 0; i < RCONF->GetNumNodes(); i++)
	{
		endpoint = RCONF->GetEndpoint(i);
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
	
	for (i = 0; i < RCONF->GetNumNodes(); i++)
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
	
	for (i = 0; i < RCONF->GetNumNodes(); i++)
	{
		if (numReplies[i] < 1)
			continue;
		double skew = totalSkew[i] / numReplies[i];

		if (verbosity)
			Log_Trace("skew for nodeID %d: %lf", i, skew);
		
		if (skew > MAX_CLOCK_SKEW && failOnSkew)
		{
			Log_Trace("skew for nodeID %d: %lf", i, skew);
			Log_Trace("  totalSkew: %lf", totalSkew[i]);
			Log_Trace("  numReplies: %d", numReplies[i]);
			STOP_FAIL("Clock skew between nodes exceeds allowed maximum", 1);
		}
	}
	
	NextSeries();
}

void TimeCheck::OnSendTimeout()
{
	unsigned i;
	
	for (i = 0; i < RCONF->GetNumNodes(); i++)
	{
		msg.Request(RCONF->GetNodeID(), series, Now());
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
		CheckNodeIdentity();
		unsigned senderID = msg.nodeID;
		msg.Response(RCONF->GetNodeID(), msg.series,
					 msg.requestTimestamp, Now());
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
		if (msg.nodeID < RCONF->GetNumNodes())
		{
			numReplies[msg.nodeID]++;
			totalSkew[msg.nodeID] += skew;
			
//			if (verbosity)
//			{
//				double skew = totalSkew[msg.nodeID] / numReplies[msg.nodeID];
//				Log_Trace("Running skew for nodeID %d after %d msgs: %lf",
//					msg.nodeID, numReplies[msg.nodeID], skew);
//			}
		}
	}
}

void TimeCheck::CheckNodeIdentity()
{
	Endpoint a, b;
	reader->GetEndpoint(a);
	
	b = RCONF->GetEndpoint(msg.nodeID);
	
	if (b.GetAddress() != INADDR_ANY && a.GetAddress() != b.GetAddress())
		STOP_FAIL("Node identity mismatch. Check all configuration files!", 0);
}
