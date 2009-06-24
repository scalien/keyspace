#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include "Endpoint.h"

#define TCP SOCK_STREAM
#define UDP SOCK_DGRAM

class Socket
{
public:
	int		fd;
	int		type;
	bool	listening;
	
	Socket() { fd = -1; listening = false; }
	
	bool Create(int type);

	bool SetNonblocking();

	bool Bind(int port, const char* interface = NULL);
	bool Listen(int port, int backlog = 1024, const char* interface = NULL);
	bool Accept(Socket* newSocket);
	bool Connect(Endpoint &endpoint);

	bool GetEndpoint(Endpoint &endpoint);

	bool SendTo(void* data, int count, const Endpoint &endpoint);
	int	 Send(const char* data, int count, int timeout = 0);
	int  Read(char* data, int count, int timeout = 0);
	
	void Close();	
};

#endif
