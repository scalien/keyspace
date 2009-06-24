#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "System/Log.h"

#define ENDPOINT_STRING_SIZE	18

class Endpoint
{
public:
	sockaddr_in	sa;

	Endpoint();
	
	bool		operator==(const Endpoint &other) const;

	bool		Set(struct sockaddr_in &sa_);
	bool		Set(const char* ip, int port);
	bool		Set(const char* ip_port);

	bool		SetPort(int port);
	int			GetPort();
	
	const char*	ToString();
private:
	char		buffer[ENDPOINT_STRING_SIZE];
};

/*
inline bool operator==(const Endpoint &a, const Endpoint &b)
{
	return a.operator==(b);
}
*/

#endif
