#include "LogQueue.h"

bool LogQueue::Push(char protocol, ByteString value)
{
	QueueItem* qi = new QueueItem();
	
	qi->protocol = protocol;
	qi->value = value;
	
	queue.Add(qi);
	
	return true;
}


QueueItem* LogQueue::Next()
{
	QueueItem** it;
	
	it = queue.Tail();
	
	if (it == NULL)
	{
		return NULL;
	}
		
	return *it;
}

QueueItem* LogQueue::Pop()
{
	QueueItem** it;
	QueueItem* qi;
	
	it = queue.Tail();
	
	if (it == NULL)
	{
		return NULL;
	}
	
	qi = *it;
	
	queue.Remove(it);
	
	return qi;
}

int LogQueue::Size()
{
	return queue.Size();
}

bool LogQueue::RemoveAll(char protocol)
{
	QueueItem** it;
	QueueItem* qi;
	
	for (it = queue.Head(); it != NULL; /* advanced in body */)
	{
		qi = *it;
		
		if (qi->protocol == protocol)
		{
			it = queue.Remove(it);
			delete qi;
		}
		else
			it = queue.Next(it);
	}
	
	return true;
}
