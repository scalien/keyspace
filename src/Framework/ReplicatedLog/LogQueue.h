#ifndef LOGQUEUE_H
#define LOGQUEUE_H

#include "System/Buffer.h"
#include "System/Containers/List.h"

class LogQueue
{
public:
	~LogQueue();

	bool				Push(ByteString& value);
	ByteString*			Next();
	ByteString*			Pop();
	int					Length();
	
private:
	List<ByteString*>	queue;
};

#endif
