#include "Common.h"

long strntol(char* buffer, int size, int* nread)
{
	bool neg;
	long n, i, digit;
	char c;

	if (buffer == NULL || size < 1)
	{
		*nread = 0;
		return 0;
	}
	
#define ADVANCE()	i++; c = buffer[i];

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
}
