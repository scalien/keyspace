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

int Socket::Send(const char* data, int count, int timeout)
{
	size_t		left;
	ssize_t		nwritten;
	timeval		tv;
	fd_set		fds;
	int			ret;
	
	left = count;
	while (left > 0)
	{
		if ((nwritten = write(fd, data, count)) < 0)
		{
			if (errno == EINTR)
			{
				nwritten = 0;
			}
			else if (errno == EPIPE)
			{
				return -1;
			}
			else if (errno == EAGAIN)
			{
				FD_ZERO(&fds);
				FD_SET(fd, &fds);
				if (timeout)
				{
					tv.tv_sec = timeout / 1000;
					tv.tv_usec = (timeout % 1000) * 1000;
					ret = select(fd + 1, NULL, &fds, NULL, &tv);
				}
				else
					ret = select(fd + 1, NULL, &fds, NULL, NULL);

				if (ret <= 0)
					return -1;

				nwritten = 0;
			}
			else
				return -1;
		}

		left -= nwritten;
		data += nwritten;
	}
	
	return count;
}

int Socket::Read(char* data, int count, int timeout)
{
	size_t		left;
	ssize_t		nread;
	timeval		tv;
	fd_set		fds;
	int			ret;
	
	left = count;
again:
	if ((nread = read(fd, data, left)) < 0)
	{
		if (errno == EINTR)
		{
			nread = 0;
			goto again;
		}
		else if (errno == EAGAIN)
		{
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			if (timeout)
			{
				tv.tv_sec = timeout / 1000;
				tv.tv_usec = (timeout % 1000) * 1000;
				ret = select(fd + 1, &fds, NULL, NULL, &tv);
			}
			else
				ret = select(fd + 1, &fds, NULL, NULL, NULL);

			if (ret <= 0)
				return -1;

			goto again;
		}
		else
			return -1;
	}
	else if (nread == 0)
	{
		// End of stream
		return 0;
	}
		
	left -= nread;
	data += nread;
	
	return count - left;
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
