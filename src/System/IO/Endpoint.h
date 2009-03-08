#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <string.h>
#include <arpa/inet.h>
#include "System/Log.h"

class Endpoint
{
public:
	struct sockaddr_in	sa;
	
	bool	Set(struct sockaddr_in &sa_);
	
	bool	Set(char* ip, int port);
	
	bool	Set(char* ip_port);
	
	bool	SetPort(int port);
		
	int		GetPort();
	
	char*	ToString();
	
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
