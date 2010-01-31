#include "Endpoint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#define s_addr S_un.S_addr
#undef SetPort
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#ifdef _WIN32
int inet_aton(const char *cp, struct in_addr *in)
{
	if (strcmp(cp, "255.255.255.255") == 0)
	{
		in->s_addr = (u_long) 0xFFFFFFFF;
		return 1;
	}
	
	in->s_addr = inet_addr(cp);
	if (in->s_addr == INADDR_NONE)
		return 0;

	return 1;
}
#endif

// TODO this is temporary, we need a real DNS resolver
static bool DNS_ResolveIpv4(const char* name, struct in_addr* addr)
{
	struct hostent* hostent;

	// FIXME gethostbyname is not multithread-safe!
	hostent = gethostbyname(name);
	if (!hostent)
		return false;

	if (hostent->h_addrtype != AF_INET)
		return false;

	addr->s_addr = *(u_long *) hostent->h_addr_list[0];

	return true;
}


Endpoint::Endpoint()
{
	struct sockaddr_in *sa = (struct sockaddr_in *) &saBuffer;

	memset((char *) sa, 0, sizeof(sa));
	sa->sin_family = 0; sa->sin_port = 0; 
	sa->sin_addr.s_addr = 0; sa->sin_zero[0] = 0;
	buffer[0] = 0;
}

bool Endpoint::operator==(const Endpoint &other) const
{
	struct sockaddr_in *sa = (struct sockaddr_in *) &saBuffer;
	struct sockaddr_in *other_sa = (struct sockaddr_in *) &other.saBuffer;

	return	sa->sin_family == other_sa->sin_family &&
			sa->sin_port == other_sa->sin_port &&
			sa->sin_addr.s_addr == other_sa->sin_addr.s_addr;
}

bool Endpoint::operator!=(const Endpoint &other) const
{
	return !operator==(other);
}

bool Endpoint::Set(const char* ip, int port, bool resolv)
{
	struct sockaddr_in *sa = (struct sockaddr_in *) &saBuffer;

	memset((char *) sa, 0, sizeof(sa));
	sa->sin_family = AF_INET;
	sa->sin_port = htons((uint16_t)port);
	if (inet_aton(ip, &sa->sin_addr) == 0)
	{
		if (resolv)
		{
			if (!DNS_ResolveIpv4(ip, &sa->sin_addr))
			{
				Log_Trace("DNS resolv failed");
				return false;
			}
			else
				return true;
		}
		
		Log_Trace("inet_aton() failed");
		return false;
	}
	return true;
}

#define MAX_IP 16

bool Endpoint::Set(const char* ip_port, bool resolv)
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

	ret = Set(ip, port, resolv);
	
	return ret;
}

bool Endpoint::SetPort(int port)
{
	struct sockaddr_in *sa = (struct sockaddr_in *) &saBuffer;

	sa->sin_port = htons((uint16_t)port);
	return true;
}

int Endpoint::GetPort()
{
	struct sockaddr_in *sa = (struct sockaddr_in *) &saBuffer;

	return ntohs(sa->sin_port);
}

uint32_t Endpoint::GetAddress()
{
	struct sockaddr_in *sa = (struct sockaddr_in *) &saBuffer;

	return ntohl(sa->sin_addr.s_addr);
}

const char* Endpoint::ToString()
{
	return ToString(buffer);
}

const char* Endpoint::ToString(char s[ENDPOINT_STRING_SIZE])
{
	struct sockaddr_in *sa = (struct sockaddr_in *) &saBuffer;

	snprintf(s, ENDPOINT_STRING_SIZE, "%s:%u",
		inet_ntoa(sa->sin_addr), ntohs(sa->sin_port));
	
	return s;
}

