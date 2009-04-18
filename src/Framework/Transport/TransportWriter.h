#ifndef TRANSPORT_WRITER_H
#define TRANSPORT_WRITER_H

class IOProcessor;
class Scheduler;
class Endpoint;
class ByteString;

class TransportWriter
{
public:
	virtual			~TransportWriter() {}

	virtual bool 	Init(Endpoint &endpoint) = 0;
	virtual	void	Write(ByteString &bs) = 0;
};

#endif
