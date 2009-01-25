#include "MemLog.h"

Entry* MemLog::Head()
{
	Entry** it;
	
	it = log.Head();
	
	if (it == NULL)
		return NULL;
	else
		return *it;
}

bool MemLog::Push(int paxosID, ByteString value)
{
	Entry* record = new Entry();
	record->paxosID = paxosID;
	if (!record->value.Set(value))
	{
		delete record;
		return false;
	}
	
	log.Add(record);
	
	return true;
}

Entry* MemLog::Get(int paxosID)
{
	Entry** it;
	Entry* record;
	
	for (it = log.Head(); it != NULL; it = log.Next(it))
	{
		record = *it;
		
		if (record->paxosID == paxosID)
			return record;
	}
	
	return NULL;
}