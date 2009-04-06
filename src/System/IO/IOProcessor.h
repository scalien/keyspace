#ifndef IOPROCESSOR_H
#define IOPROCESSOR_H

#include "IOOperation.h"
#include "System/Common.h"

class Callable;

class IOProcessor
{
public:
	static bool Init();
	static void Shutdown();

	static bool Add(IOOperation* ioop);
	static bool Remove(IOOperation* ioop);

	static bool Poll(int sleep);

	static bool Complete(Callable* callable);
};

#endif
