#ifndef MEMOMSG_H
#define MEMOMSG_H

#include "MemoClientMsg.h"
#include "Buffer.h"

class MemoMsg : public MemoClientMsg
{
public:
	int				nodeID;
	long			msgID;
	
	static int		Read(char* buf, int size, MemoMsg* msg);
	static int		Write(MemoMsg* msg, char* buf, int size);
};

#endif