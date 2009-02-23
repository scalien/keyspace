#ifndef LOGQUEUE_H
#define LOGQUEUE_H

#include "Framework/ReplicatedLog/LogCache.h"

class QueueItem
{
public:
	char					protocol;
	ByteString				value;
};


class LogQueue
{
public:
	bool					Push(char protocol, ByteString value);
	QueueItem*				Next();
	QueueItem*				Pop();
	int						Size();
	
	bool					RemoveAll(char protocol);

private:
	List<QueueItem*>		queue;
};

#endif