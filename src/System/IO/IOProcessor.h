#ifndef IOPROCESSOR_H
#define IOPROCESSOR_H

#include "IOOperation.h"
#include "System/Common.h"

class IOProcessor
{
public:
	static IOProcessor* New();

	bool Init();
	void Shutdown();
	
	bool Add(IOOperation* ioop);
	bool Remove(IOOperation* ioop);
	
	bool Complete(AsyncOperation *aop);
	
	bool Poll(int sleep);
};

#endif
