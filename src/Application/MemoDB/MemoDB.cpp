#include "MemoDB.h"

MemoDB::MemoDB() :
onAppend(this, &MemoDB::OnAppend)
{
	msgID = 0;
//	ReplicatedLog::SetAppendCallback(PROTOCOL_MEMODB, &onAppend);
// TODO
}

// called by the client to add a command to the log and have it executed
void MemoDB::Add(ByteString command, Callable* onComplete)
{
	Job	j;

	j.command = command;
	j.onComplete = onComplete;
	
	if (appending)
		backlog.Add(j);
	else
	{
		job = j;
		ReplicatedLog::Append(command);
	}
}

void MemoDB::OnAppend()
{
// called back by the replicated log when a new value is appended 
// this may not be our client's

	Entry* entry;
	
	entry = logCache.Last();
	
	Execute(entry->value);
	
	if (backlog.size > 0)
	{
		job = *(backlog.Head());
		if (ReplicatedLog::Append(job.command))
			backlog.Remove(job);
	}
}

void MemoDB::Execute(ByteString command)
{
	bool callback = false;
	
	if (appending)
		if (job.command == command)
			callback = true;

	MemoClientMsg::Read(command, &msg);
	
	if (msg.type == GET_REQUEST && callback)
		Get();
	else if (msg.type == SET_REQUEST)
		Set();
	else if (msg.type == TESTANDSET_REQUEST)
		TestAndSet();
	
	if (callback)
		Call(job.onComplete);
}

void MemoDB::Get()
{
	Record* it;
	
	for (it = entries.Head(); it != NULL; it = entries.Next(it))
	{
		if (it->key == msg.key)
		{
			msg.value = it->value;
			msg.GetResponse(EXEC_OK, RESPONSE_OK);
			return;
		}
	}

	msg.GetResponse(EXEC_OK, RESPONSE_FAIL);
	
}

void MemoDB::Set()
{
	Record* it;
	
	for (it = entries.Head(); it != NULL; it = entries.Next(it))
	{
		if (it->key == msg.key)
		{
			it->value = msg.value;
			msg.SetResponse(EXEC_OK, RESPONSE_OK);
			return;
		}
	}

	Record entry;
	entry.key = msg.key;
	entry.value = msg.value;
	entries.Add(entry);
	
	msg.SetResponse(EXEC_OK, RESPONSE_OK);
}

void MemoDB::TestAndSet()
{
	Record* it;
	
	for (it = entries.Head(); it != NULL; it = entries.Next(it))
	{
		if (it->key == msg.key)
		{
			if (it->value == msg.test)
			{
				it->value = msg.value;
				msg.TestAndSetResponse(EXEC_OK, RESPONSE_OK);
				return;
			}
			else
			{
				msg.TestAndSetResponse(EXEC_OK, RESPONSE_FAIL);
				return;
			}
		}
	}
	
	msg.TestAndSetResponse(EXEC_OK, RESPONSE_FAIL);
}