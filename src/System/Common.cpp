#include "Common.h"
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>

int64_t strntoint64_t(const char* buffer, int length, unsigned* nread)
{
	bool	neg;
	long	i, digit;
	int64_t	n;
	char	c;
	
#define ADVANCE()	i++; c = buffer[i];

	if (buffer == NULL || length < 1)
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
	
	while(c >= '0' && c <= '9' && i < length)
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

uint64_t strntouint64_t(const char* buffer, int length, unsigned* nread)
{
	long		i, digit;
	uint64_t	n;
	char		c;

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

const char* rprintf(const char* format, ...)
{
	static char buffer[8*1024];
	va_list		ap;
	
	va_start(ap, format);
	vsprintf(buffer, format, ap);
	
	return buffer;
}

void* Alloc(int num, int size)
{
	if (num == 0 || size == 0)
		return NULL;
		
	return malloc(num * size);
}
