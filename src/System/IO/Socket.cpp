#include "Socket.h"
#include "System/Log.h"
#ifdef PLATFORM_DARWIN
#include <machine/types.h>
#endif
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

bool Socket::Create(int type_)
{
	int ret;
	int	sockopt;
	
	if (fd >= 0)
	{
		Log_Message("Called Create() on existing socket");
		return false;
	}
	
	fd = socket(AF_INET, type_, 0);
	if (fd < 0)
	{
		Log_Errno();
		return false;
	}
	
	sockopt = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
	if (ret < 0)
	{
		Log_Errno();
		Close();
		return false;
	}
	
	type = type_;
	listening = false;
	
	return true;
}

bool Socket::SetNonblocking()
{
	int ret;
	
	if (fd < 0)
	{
		Log_Message("SetNonblocking on invalid file descriptor");
		return false;
	}
	
	ret = fcntl(fd, F_SETFL, FNONBLOCK | FASYNC);
	if (ret < 0)
	{
		Log_Errno();
		return false;
	}
	
	return true;
}

bool Socket::Bind(int port)
{
	int					ret;
	struct sockaddr_in	sa;
	
	memset(&sa, 0, sizeof(sa));
	sa.sin_port = htons((uint16_t)port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	
	ret = bind(fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in));
	if (ret < 0)
	{
		Log_Errno();
		Close();
		return false;
	}
	
	return true;
}

bool Socket::Listen(int port, int backlog)
{
	int	ret;
	
	if (!Bind(port))
		return false;
	
	ret = listen(fd, backlog);
	if (ret < 0)
	{
		Log_Errno();
		Close();
		return false;
	}
	
	listening = true;
	
	return true;
}

bool Socket::Accept(Socket *newSocket)
{
	newSocket->fd = accept(fd, NULL, NULL);
	if (newSocket->fd < 0)
	{
		Log_Errno();
		Close();
		return false;
	}
	
	return true;
}

bool Socket::Connect(Endpoint &endpoint)
{
	int ret;
	
	ret = connect(fd, (struct sockaddr *) &endpoint.sa, sizeof(endpoint.sa));
	
	if (ret < 0)
	{
		if (errno != EINPROGRESS)
		{
			Log_Errno();
			return false;
		}
	}

	return true;
}

bool Socket::GetEndpoint(Endpoint &endpoint)
{
	int ret;
	socklen_t len = sizeof(endpoint.sa);
	
	ret = getpeername(fd, (sockaddr*) &endpoint.sa, &len);
	
	if (ret < 0)
	{
		Log_Errno();
		Close();
		return false;
	}
	
	return true;
}

bool Socket::SendTo(void *data, int count, const Endpoint &endpoint)
{
	int ret;
	
	ret = sendto(fd, data, count, 0,
				 (const sockaddr *) &endpoint.sa,
				 sizeof(sockaddr));
	
	if (ret < 0)
	{
		Log_Errno();
		return false;
	}
	
	return true;
}

void Socket::Close()
{
	int ret;
	
	if (fd != -1)
	{
		ret = close(fd);

		if (ret < 0)
			Log_Errno();

		fd = -1;
	}
}
