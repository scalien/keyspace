#ifndef MEMODB_H
#define MEMODB_H

#include "Callable.h"
#include "ReplicatedLog.h"
#include "MemoMsg.h"

typedef struct
{
	ByteString	key;
	ByteString	value;
} Record;

inline bool operator==(Record &a, Record &b)
{
	return a.key == b.key && a.key == b.key;
}

typedef struct
{
	ByteString	command;
	Callable*	onComplete;
} Job;

inline bool operator==(Job &a, Job &b)
{
	return a.command == b.command && a.onComplete == b.onComplete;
}

class MemoDB : public ReplicatedLog
{
public:
	MemoDB();
	
	Job				job;
	List<Job>		backlog;
	
	List<Record>	entries; // the database
	
	long			msgID;
	MemoMsg			msg;

	void			Add(ByteString command, Callable* onComplete);
	
	void			Execute(ByteString command);
	
	void			Get();
	void			Set();
	void			TestAndSet();
	
	void			OnAppend();
	MFunc<MemoDB>	onAppend;
};

#endif