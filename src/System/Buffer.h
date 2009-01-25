#ifndef BUFFER_H
#define BUFFER_H

#include "Common.h"

class ByteString
{
public:
	int		size;
	int		length;
	char*	buffer;
	
	ByteString() : size(0), length(0), buffer(NULL) {}
	
	ByteString(int size_, int length_, char* buffer_) : size(size_), length(length_), buffer(buffer_) {}
	
	bool operator==(const ByteString& other)
	{
		return MEMCMP(buffer, length, other.buffer, other.length);
	}
};

template<int n>
class ByteArray : public ByteString
{
public:
	char	data[n];
	
	ByteArray() { size = n; length = 0; buffer = data; }
			
	bool Set(ByteString bs)
	{
		if (bs.length > size)
			return false;
		
		memcpy(buffer, bs.buffer, bs.length);
		
		length = bs.length;
		
		return true;
	}
};

#endif