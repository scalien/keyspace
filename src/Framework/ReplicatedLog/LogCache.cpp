#include "LogCache.h"

#define INDEX(n) (((n) + size) % size)

Entry* LogCache::Last()
{
	if (count != 0)
		return &array[INDEX(next - 1)];
	else
		return NULL;
}

bool LogCache::Push(ulong64 paxosID, ByteString value)
{
	array[next].paxosID = paxosID;
	array[next].value.Set(value);

	next = INDEX(next + 1);
	
	if (count < size) count++;
	
	return true;
}

Entry* LogCache::Get(ulong64 paxosID)
{
	if (count == 0)
		return NULL;
	
	int tail = INDEX(next - count);
	int head = INDEX(next - 1);
	
	if (paxosID < array[tail].paxosID)
		return NULL;
	
	if (array[head].paxosID < paxosID)
		return NULL;
	
	int offset = paxosID - array[tail].paxosID;
	return &array[INDEX(tail + offset)];
}
