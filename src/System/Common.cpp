#include "Common.h"
#include <stdarg.h>
#include <stdio.h>

long strntol(char* buffer, int size, int* nread)
{
	bool neg;
	long n, i, digit;
	char c;
	
#define ADVANCE()	i++; c = buffer[i];

	if (buffer == NULL || size < 1)
	{
		*nread = 0;
		return 0;
	}
	
	n = 0;
	i = 0;
	c = *buffer;
	
	if (c == '-')
	{
		neg = true;
		ADVANCE();
	}
	else
		neg = false;
	
	while(c >= '0' && c <= '9' && i < size)
	{
		digit = c - '0';
		n = n * 10 + digit;
		ADVANCE();
	}
	
	if (neg && i == 1)
	{
		*nread = 0;
		return 0;
	}
	
	*nread = i;

	if (neg)
		return -n;
	else
		return n;

#undef ADVANCE
}

ulong64 strntoulong64(char* buffer, int length, int* nread)
{
	long i, digit;
	ulong64 n;
	char c;

#define ADVANCE()	i++; c = buffer[i];	

	if (buffer == NULL || length < 1)
	{
		*nread = 0;
		return 0;
	}

	n = 0;
	i = 0;
	c = *buffer;
	
	while(c >= '0' && c <= '9' && i < length)
	{
		digit = c - '0';
		n = n * 10 + digit;
		ADVANCE();
	}
	
	*nread = i;

	return n;

#undef ADVANCE
}

char* rprintf(const char* format, ...)
{
	static char buffer[1024];
	va_list		ap;
	
	va_start(ap, format);
	vsprintf(buffer, format, ap);
	
	return buffer;
}
