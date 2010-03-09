#ifndef KEYSPACE_CLIENT_WRAP_H
#define KEYSPACE_CLIENT_WRAP_H

#include <string>
#include <list>

typedef void * Client;
typedef void * Result;

struct ReturnValue
{
	std::string	value;
	int			status;
};

Client Create();

int			Init(Client client, std::list<std::string> nodes);
ReturnValue Get(Client client, std::string key);
//int			Set(Client client, std::string &key, std::string &value);

#endif
