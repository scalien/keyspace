#include "LogQueue.h"

LogQueue::~LogQueue()
{
	ByteString**	it;
	ByteString*		bs;
	
	for (it = queue.Head(); it; )
	{
		bs = *it;
		it = queue.Next(it);
		delete bs;
	}
}

bool LogQueue::Push(ByteString& value)
{
	ByteString* bs = new ByteString();

	bs->Set(value);

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

void LogQueue::Clear()
{
	queue.Clear();
}

int LogQueue::Length()
{
	return queue.Length();
}
