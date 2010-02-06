#ifndef LOGQUEUE_H
#define LOGQUEUE_H

#include "System/Buffer.h"
#include "System/Containers/List.h"

class LogQueue
{
public:
	bool				Push(ByteString value);
	ByteString*			Next();
	ByteString*			Pop();
	int					Length();
	
private:
	List<ByteString*>	queue;
};

#endif
