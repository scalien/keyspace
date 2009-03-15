#include "LogQueue.h"

bool LogQueue::Push(ByteString value)
{
	ByteString* bs = new ByteString();

	*bs = value;

	queue.Add(bs);
	
	return true;
}


ByteString* LogQueue::Next()
{
	ByteString** it;
	
	it = queue.Tail();
	
	if (it == NULL)
		return NULL;
		
	return *it;
}

ByteString* LogQueue::Pop()
{
	ByteString** it;
	ByteString* bs;
	
	it = queue.Tail();
	
	if (it == NULL)
		return NULL;
	
	bs = *it;
	
	queue.Remove(bs);
	
	return bs;
}

int LogQueue::Size()
{
	return queue.Size();
}
