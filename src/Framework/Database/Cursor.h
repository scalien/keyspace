#ifndef CURSOR_HPP
#define CURSOR_HPP

#include "System/Buffer.h"

class Cursor
{
public:
	virtual ~Cursor() {}
	
	virtual bool	Next(ByteString &key, ByteString &value) = 0;
	virtual bool	Close() = 0;
};

#endif
