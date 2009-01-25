#include "MemoMsg.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int MemoMsg::Read(char* buf, int size, MemoMsg* msg) // todo: should be called length
{
	// not required
	
	return -1;
}

int MemoMsg::Write(MemoMsg* msg, char* buf, int size)
{
	// this just appends
	
	int required = snprintf(buf, size, "@%d@%d@", msg->nodeID, msg->msgID);
	
	if (required > size)
		return -1;
	
	return required;
}
