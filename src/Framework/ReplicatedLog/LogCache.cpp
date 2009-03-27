#include <stdlib.h>
#include "LogCache.h"

#define INDEX(n) (((n) + size) % size)

LogCache::LogCache()
{
	count = 0;
	next = 0;
	size = SIZE(logItems);
	allocated = 0;
}

LogCache::~LogCache()
{
	for (int i = 0; i < size; i++)
		logItems[i].value.Free();
}

/*LogItem* LogCache::Last()
{
	if (count != 0)
		return &logItems[INDEX(next - 1)];
	else
		return NULL;
}*/

bool LogCache::Push(ulong64 paxosID, ByteString value)
{
	int tail, head, i;
	
	tail = INDEX(next - count);
	head = INDEX(next - 1);

	if (allocated - logItems[next].value.size + value.length > MAX_MEM_USE)
	{
		// start freeing up at the tail
		for (i = tail; i <= head; i = INDEX(i+1))
		{
			allocated -= logItems[i].value.size;
			logItems[i].value.Free();
			count--;
			if (allocated + value.length <= MAX_MEM_USE)
				break;
		}
	}
	
	logItems[next].paxosID = paxosID;
	allocated -= logItems[next].value.size;
	logItems[next].value.Free();
	if (!logItems[next].value.Allocate(value.length))
		ASSERT_FAIL();
	allocated += logItems[next].value.size;
	logItems[next].value.Set(value);
	next = INDEX(next + 1);
	
	if (count < size)
		count++;
	
	return true;
}

bool LogCache::Get(ulong64 paxosID, ByteString& value)
{
	int tail, head, offset;
	
	if (count == 0)
		return false;
	
	tail = INDEX(next - count);
	head = INDEX(next - 1);
	
	if (paxosID < logItems[tail].paxosID)
		return false;
	
	if (logItems[head].paxosID < paxosID)
		return false;
	
	offset = paxosID - logItems[tail].paxosID;

	value = logItems[INDEX(tail + offset)].value;
	return true;
}
