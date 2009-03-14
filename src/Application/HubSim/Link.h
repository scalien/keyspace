#ifndef LINK_H
#define LINK_H

#include <stdlib.h>
#include <time.h>
#include "System/IO/Endpoint.h"

#define MAX_UDP_PACKET_SIZE 65507

class Port
{
public:
	virtual void Write(Endpoint &ep, const char* data, int count) = 0;
};


class Link
{
public:	
	virtual ~Link() {}
	virtual void WriteUDP(Port* port, Endpoint &dst, const char* data, int count)
	{
		port->Write(dst, data, count);
	}
};


class PacketLossLink : public Link
{
public:
	double prob;
	
	PacketLossLink()
	{
		prob = 0;
		srand(time(NULL));
	}

	void SetLossProbability(double prob_)
	{
		prob = prob_;
	}
	
	virtual void WriteUDP(Port* port, Endpoint &dst, const char *data, int count)
	{
		double rnd;
		rnd = random() / (double) RAND_MAX;
		if (rnd >= prob)
			port->Write(dst, data, count);
	}
};


class ReorderingLink : public Link
{
public:
	class UDPBuffer : public ByteArray<MAX_UDP_PACKET_SIZE>
	{
	public:
		UDPBuffer*	next;
		Endpoint	endpoint;
		UDPBuffer() : ByteArray<MAX_UDP_PACKET_SIZE>()
		{
			next = NULL;
		}
	};
	
	enum { MAX_NBUFFER = 10 };
	double prob;
	UDPBuffer* buffers;
	UDPBuffer* last;
	int numBuffers;
	
	ReorderingLink()
	{
		prob = 0;
		numBuffers = 0;
		buffers = NULL;
		last = NULL;
		srand(time(NULL));
	}
	
	void SetReorderProbability(double prob_)
	{
		prob = prob_;
	}
	
	void AppendData(Endpoint &dst, const char* data, int count)
	{
		UDPBuffer* buf;
		
		buf = new UDPBuffer;
		buf->Set((char*) data, count);
		buf->endpoint = dst;
		if (!last)
		{
			buffers = buf;
			last = buf;
		}
		else
		{
			last->next = buf;
			last = buf;
		}
	}
	
	void WriteFirst(Port* port)
	{
		UDPBuffer* buf;
		
		buf = buffers;
		if (!buf)
			return;
		
		if (buf == last)
			last = NULL;

		buffers = buf->next;
		Link::WriteUDP(port, buf->endpoint, buf->data, buf->length);
		delete buf;
	}
	
	virtual void WriteUDP(Port* port, Endpoint &dst, const char* data, int count)
	{
		double rnd;
		rnd = random() / (double) RAND_MAX;
		if (rnd < prob)
		{
			AppendData(dst, data, count);
		}
		else if (buffers)
		{
			WriteFirst(port);
			Link::WriteUDP(port, dst, data, count);
		}
		else
		{
			Link::WriteUDP(port, dst, data, count);
		}
	}
};

class RandomErrorLink : public Link
{
public:
	PacketLossLink	plLink;
	ReorderingLink	prLink;
	
	RandomErrorLink()
	{
		plLink.SetLossProbability(0.5);
		prLink.SetReorderProbability(0.5);
	}
	
	virtual void WriteUDP(Port* port, Endpoint &dst, const char* data, int count)
	{
		double rnd;
		rnd = random() / (double) RAND_MAX;
		if (rnd <= 0.33)
		{
			Link::WriteUDP(port, dst, data, count);
		}
		else if (rnd <= 0.66)
		{
			plLink.WriteUDP(port, dst, data, count);
		}
		else
		{
			prLink.WriteUDP(port, dst, data, count);
		}
	}
	
	
};

#endif
