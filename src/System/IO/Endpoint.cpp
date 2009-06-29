#include "Endpoint.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

Endpoint::Endpoint()
{
	bzero(&sa, sizeof(sa));
	sa.sin_family = 0; sa.sin_port = 0; 
	sa.sin_addr.s_addr = 0; sa.sin_zero[0] = 0;
	buffer[0] = 0;
}

bool Endpoint::operator==(const Endpoint &other) const
{
	return	sa.sin_family == other.sa.sin_family &&
			sa.sin_port == other.sa.sin_port &&
			sa.sin_addr.s_addr == other.sa.sin_addr.s_addr;
}

bool Endpoint::operator!=(const Endpoint &other) const
{
	return !operator==(other);
}

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
		Log_Trace("inet_aton() failed");
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
		Log_Trace("No ':' in host specification");
		return false;
	}
	
	memcpy(ip, ip_port, p - ip_port);
	ip[p - ip_port] = '\0';
	p++;
	
	port = -1;
	port = atoi(p);
	if (port < 1 || port > 65535)
	{
		Log_Trace("atoi() failed to produce a sensible value");
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

uint32_t Endpoint::GetAddress()
{
	return sa.sin_addr.s_addr;
}

const char* Endpoint::ToString()
{
	return ToString(buffer);
}

const char* Endpoint::ToString(char s[ENDPOINT_STRING_SIZE])
{
	snprintf(s, ENDPOINT_STRING_SIZE, "%s:%u", inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
	return s;
}

