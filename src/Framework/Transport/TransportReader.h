#ifndef TRANSPORTREADER_H
#define TRANSPORTREADER_H

#include "System/IO/IOProcessor.h"
#include "System/Buffer.h"
#include "System/Events/Callable.h"

class TransportReader
{
public:
	virtual			~TransportReader() {}

	virtual bool	Init(int port_) = 0;

	virtual void	SetOnRead(Callable* onRead) = 0;
	virtual void	GetMessage(ByteString& bs) = 0;
	virtual void	Stop() = 0;
	virtual void	Continue() = 0;
};

#endif
