#include "KeyspaceClientWrap.h"
#include "KeyspaceClient.h"

Client Create()
{
	return new Keyspace::Client();
}

int	Init(Client client_, std::list<std::string> nodes)
{
	Keyspace::Client *client = (Keyspace::Client *) client_;
	int	status;
	const char **nodev;
	int nodec;
	int i;
	
	nodec = nodes.size();
	nodev = new const char*[nodec];
	i = 0;
	for (std::list<std::string>::iterator it = nodes.begin(); it != nodes.end(); it++)
	{
		nodev[i] = (*it).c_str();
		i++;
	}
	
	status = client->Init(nodec, nodev);
	delete[] nodev;
	
	return status;
}

ReturnValue Get(Client client_, std::string &key_)
{
	Keyspace::Client *client = (Keyspace::Client *) client_;
	ByteString key;
	ByteString value;
	ReturnValue retval;
	Keyspace::Result *result;
	
	key.buffer = (char*) key_.c_str();
	key.size = key_.length();
	key.length = key_.length();
	
	retval.status = client->Get(key);
	result = client->GetResult();
	result->Value(value);
	
	retval.value.append(value.buffer, value.length);
	
	return retval;
}
