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
		logItems[i].Free();
}

bool LogCache::Push(uint64_t paxosID_, ByteString value)
{
	int tail, head, i;
	
	tail = INDEX(next - count);
	head = INDEX(next - 1);

	if (allocated - logItems[next].size + value.length > MAX_MEM_USE)
	{
		// start freeing up at the tail
		for (i = tail; i <= head; i = INDEX(i+1))
		{
			allocated -= logItems[i].size;
			logItems[i].Free();
			count--;
			if (allocated + value.length <= MAX_MEM_USE)
				break;
		}
	}
	
	allocated -= logItems[next].size;
	logItems[next].Free();
	if (!logItems[next].Allocate(value.length))
		ASSERT_FAIL();
	allocated += logItems[next].size;
	logItems[next].Set(value);
	next = INDEX(next + 1);
	
	if (count < size)
		count++;
	
	paxosID = paxosID_;
	
	return true;
}

bool LogCache::Get(uint64_t paxosID_, ByteString& value)
{
	int tail, head, offset;
	
	if (count == 0)
		return false;
	
	tail = INDEX(next - count);
	head = INDEX(next - 1);
	
	if (paxosID_ <= paxosID - count)
		return false;
	
	if (paxosID < paxosID_)
		return false;
	
	offset = paxosID_ - (paxosID - count + 1);

	value = logItems[INDEX(tail + offset)];
	return true;
}
