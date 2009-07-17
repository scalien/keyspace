#ifndef TIMECHECK_H
#define TIMECHECK_H

#include "System/Events/Timer.h"
#include "Framework/Transport/TransportUDPReader.h"
#include "Framework/Transport/TransportUDPWriter.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
#include "TimeCheckMsg.h"

#define TIMECHECK_PORT_OFFSET	3
#define NUMMSGS					60
#define SERIES_TIMEOUT			60*1000 // run every 60 seconds

class TimeCheck
{
public:
	TimeCheck();

	void				Init(bool verbosity_ = false, bool failOnSkew_ = true);

	void				OnRead();
	void				OnSeriesTimeout();
	void				OnSendTimeout();
	void				NextSeries();
	void				CheckNodeIdentity();
	
private:
	void				InitTransport();
	
	TransportUDPReader*	reader;
	TransportUDPWriter**writers;
	MFunc<TimeCheck>	onRead;
	TimeCheckMsg		msg;
	
	MFunc<TimeCheck>	onSeriesTimeout;
	MFunc<TimeCheck>	onSendTimeout;
	CdownTimer			seriesTimeout;
	CdownTimer			sendTimeout;
	
	ByteArray<1024>		data;
	uint64_t			series;
	unsigned			sentInSeries;
	int*				numReplies;
	double*				totalSkew;
	bool				verbosity;
	bool				failOnSkew;
};

#endif
