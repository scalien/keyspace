#include "Endpoint.h"
#include <stdlib.h>

bool Endpoint::Init(struct sockaddr_in &sa_)
{
	sa = sa_;
	return true;
}

bool Endpoint::Init(char* ip, int port)
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

bool Endpoint::Init(char* ip_port)
{
	char*	p;
	int		port;
	bool	ret;
	
	p = ip_port;
	
	while (*p != '\0' && *p != ':')
		p++;
	
	if (*p == '\0')
	{
		Log_Message("No ':' in host specification");
		return false;
	}
	
	*p = '\0';
	p++;
	
	port = -1;
	port = atoi(p);
	if (port < 1 || port > 65535)
	{
		Log_Message("atoi() failed to produce a sensible value");
		return false;
	}

	ret = Init(ip_port, port);
	
	p--;
	*p = ':';
	
	return ret;
}

char* Endpoint::ToString()
{
	sprintf(buffer, "%s:%u", inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
	return buffer;
}

int Endpoint::Port()
{
	return ntohs(sa.sin_port);ntohs(sa.sin_port);
}
