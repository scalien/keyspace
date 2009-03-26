#ifndef IOPROCESSOR_H
#define IOPROCESSOR_H

#include "IOOperation.h"
#include "System/Common.h"

class Callable;

class IOProcessor
{
public:
	static IOProcessor* Get();

	bool Init();
	void Shutdown();
	
	bool Add(IOOperation* ioop);
	bool Remove(IOOperation* ioop);
		
	bool Poll(int sleep);

	static bool Complete(Callable* callable);
};

#endif
