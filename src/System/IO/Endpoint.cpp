#include "Endpoint.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

bool Endpoint::Set(struct sockaddr_in &sa_)
{
	sa = sa_;
	return true;
}

bool Endpoint::Set(const char* ip, int port)
{
	memset((char *) &sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons((uint16_t)port);
	if (inet_aton(ip, &sa.sin_addr) == 0)
	{
		Log_Message("inet_aton() failed");
		return false;
	}
	return true;
}

#define MAX_IP 16

bool Endpoint::Set(const char* ip_port)
{
	const char*	p;
	int			port;
	bool		ret;
	char		ip[MAX_IP];
	
	p = ip_port;
	
	while (*p != '\0' && *p != ':')
		p++;
	
	if (*p == '\0')
	{
		Log_Message("No ':' in host specification");
		return false;
	}
	
	memcpy(ip, ip_port, p - ip_port);
	ip[p - ip_port] = '\0';
	p++;
	
	port = -1;
	port = atoi(p);
	if (port < 1 || port > 65535)
	{
		Log_Message("atoi() failed to produce a sensible value");
		return false;
	}

	ret = Set(ip, port);
	
	return ret;
}

bool Endpoint::SetPort(int port)
{
	sa.sin_port = htons((uint16_t)port);
	return true;
}

int Endpoint::GetPort()
{
	return ntohs(sa.sin_port);
}

char* Endpoint::ToString()
{
	snprintf(buffer, sizeof(buffer), "%s:%u", inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
	return buffer;
}
