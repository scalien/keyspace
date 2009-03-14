#ifndef TRANSPORT_WRITER_H
#define TRANSPORT_WRITER_H

class TransportWriter
{
public:
	virtual void	Init(IOProcessor* ioproc_, Endpoint &endpoint_) = 0;
	virtual	void	Write(ByteString &bs) = 0;
};

#endif
