#include <stdlib.h>
#include "LogCache.h"

#define INDEX(n) (((n) + size) % size)

LogCache::LogCache()
{
	count = 0;
	next = 0;
	size = SIZE(logItems);
	
	for (int i = 0; i < SIZE(buffers); i++)
	{
		buffers[i] = (char*)malloc(65*KB);
		logItems[i].value = ByteString(65*KB, 0, buffers[i]);
	}
}

LogCache::~LogCache()
{
	for (int i = 0; i < SIZE(buffers); i++)
		free(buffers[i]);
}

LogItem* LogCache::Last()
{
	if (count != 0)
		return &logItems[INDEX(next - 1)];
	else
		return NULL;
}

bool LogCache::Push(ulong64 paxosID, ByteString value)
{
	logItems[next].paxosID = paxosID;
	logItems[next].value = value;

	next = INDEX(next + 1);
	
	if (count < size) count++;
	
	return true;
}

LogItem* LogCache::Get(ulong64 paxosID)
{
	if (count == 0)
		return NULL;
	
	int tail = INDEX(next - count);
	int head = INDEX(next - 1);
	
	if (paxosID < logItems[tail].paxosID)
		return NULL;
	
	if (logItems[head].paxosID < paxosID)
		return NULL;
	
	int offset = paxosID - logItems[tail].paxosID;

	return &logItems[INDEX(tail + offset)];
}
