#include "Common.h"
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <inttypes.h>

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
	vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);
	
	return buffer;
}

void* Alloc(int num, int size)
{
	if (num == 0 || size == 0)
		return NULL;
		
	return malloc(num * size);
}

int snwritef(char* buffer, unsigned size, const char* format, ...)
{
	char		c;
	int			d;
	int			n;
	int64_t		i64;
	uint64_t	u64;
	char*		p;
	unsigned	length;
	va_list		ap;	
	unsigned	written;
	char		local[64];

#define ADVANCE(f, b)	{ format += f; buffer += b; size -= b; written += b; }
#define EXIT()			{ written = -1; break; }
#define REQUIRE(n)		{ if (size < n) EXIT(); }

	written = 0;

	va_start(ap, format);

	while(format[0] != '\0')
	{
		if (format[0] == '%')
		{
			if (format[1] == '\0')
				EXIT(); // % cannot be at the end of the format string
			
			if (format[1] == '%') // %%
			{
				REQUIRE(1);
				buffer[0] = '%';
				ADVANCE(2, 1);
			} else if (format[1] == 'c') // %c
			{
				REQUIRE(1);
				c = va_arg(ap, int);
				buffer[0] = c;
				ADVANCE(2, 1);
			} else if (format[1] == 'd') // %d
			{
				d = va_arg(ap, int);
				n = snprintf(local, sizeof(local), "%d", d);
				if (n < 0 || (unsigned)n > size)
					EXIT();
				memcpy(buffer, local, n);
				ADVANCE(2, n);
			} else if (format[1] == 'I') // %i to print an int64_t 
			{
				i64 = va_arg(ap, int64_t);
				n = snprintf(local, sizeof(local), "%" PRIi64 "", i64);
				if (n < 0 || (unsigned)n > size)
					EXIT();
				memcpy(buffer, local, n);
				ADVANCE(2, n);
			} else if (format[1] == 'U') // %u tp print an uint64_t
			{
				u64 = va_arg(ap, uint64_t);
				n = snprintf(local, sizeof(local), "%" PRIu64 "", u64);
				if (n < 0 || (unsigned)n > size)
					EXIT();
				memcpy(buffer, local, n);
				ADVANCE(2, n);
			} else if (format[1] == 'B') // %b to print a buffer
			{
				length = va_arg(ap, unsigned);
				p = va_arg(ap, char*);
				REQUIRE(length);
				memcpy(buffer, p, length);
				ADVANCE(2, length);
			}
		}
		else
		{
			REQUIRE(1);
			buffer[0] = format[0];
			ADVANCE(1, 1);
		}
	}
	
	va_end(ap);
	
	return written;

#undef ADVANCE
#undef EXIT
#undef REQUIRE
}
