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

	virtual void	Init(IOProcessor* ioproc_, Scheduler* scheduler_, Endpoint &endpoint_) = 0;

	virtual	void	Write(ByteString &bs) = 0;
};

#endif
