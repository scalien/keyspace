#ifndef LOGQUEUE_H
#define LOGQUEUE_H

#include "Framework/ReplicatedLog/LogCache.h"

class LogQueue
{
public:
	bool					Push(ByteString value);
	ByteString*				Next();
	ByteString*				Pop();
	int						Length();
	
private:
	List<ByteString*>		queue;
};

#endif
