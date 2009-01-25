#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <string.h>
#include <arpa/inet.h>
#include "System/Log.h"

class Endpoint
{
public:
	struct sockaddr_in	sa;
	
	bool Init(struct sockaddr_in &sa_);
	
	bool Init(char* ip, int port);
	
	bool Init(char* ip_port);
	
	char* ToString();
	
	int Port();
	
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
