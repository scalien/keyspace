#ifndef BUFFER_H
#define BUFFER_H

#include "Common.h"

class ByteString
{
public:
	int		size;
	int		length;
	char*	buffer;
	
	ByteString() { Init(); }
	
	ByteString(int size_, int length_, char* buffer_)
		: size(size_), length(length_), buffer(buffer_) {}
	
	void Init() { size = 0; length = 0; buffer = 0; }
	
	bool Set(char* str)
	{
		int len = strlen(str);
		if (len > size)
			return false;
		
		memcpy(buffer, str, len);
		
		length = len;
		
		return true;
	}
	
	bool operator==(const ByteString& other)
	{
		return MEMCMP(buffer, length, other.buffer, other.length);
	}
	
	void Clear() { length = 0; }
};

template<int n>
class ByteArray : public ByteString
{
public:
	char	data[n];
	
	ByteArray() { size = n; length = 0; buffer = data; }
	
	ByteArray(char* str)
	{
		size = n;
		length = strlen(str);
		memcpy(data, str, length);
		buffer = data;
	}
	
	bool Set(char* str) { return ByteString::Set(str); }
							
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
