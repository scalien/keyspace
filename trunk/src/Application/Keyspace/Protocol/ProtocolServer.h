#ifndef PROTOCOLSERVER_H
#define PROTOCOLSERVER_H

class ProtocolServer
{
public:
	virtual ~ProtocolServer() {}

	virtual void	Stop() = 0;
	virtual void	Continue() = 0;
};

#endif
