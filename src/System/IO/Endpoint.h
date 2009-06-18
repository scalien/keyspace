#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "System/Log.h"

class Endpoint
{
public:
	struct sockaddr_in	sa;

	Endpoint() 
	{
		sa.sin_family = 0; sa.sin_port = 0; 
		sa.sin_addr.s_addr = 0; sa.sin_zero[0] = 0;
	}
	
	bool		Set(struct sockaddr_in &sa_);
	bool		Set(const char* ip, int port);
	bool		Set(const char* ip_port);

	bool		SetPort(int port);
	int			GetPort();
	
	const char*	ToString();
	
private:
	char			buffer[32];

};

inline bool operator==(Endpoint &a, Endpoint &b)
{
	return	a.sa.sin_family == b.sa.sin_family &&
			a.sa.sin_port == b.sa.sin_port &&
			a.sa.sin_addr.s_addr == b.sa.sin_addr.s_addr;
}

#endif
