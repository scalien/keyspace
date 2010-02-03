#include "UrlParam.h"
#include "System/Buffer.h"

#include <stdarg.h>

static const char* UrlParam_ParseVarArg(const char* s, char sep, unsigned num, va_list ap)
{
	unsigned	length;
	ByteString* bs;

	while (*s != '\0' && num > 0)
	{
		bs = va_arg(ap, ByteString*);
		bs->buffer = (char*) s;
		length = 0;
		while(*s != '\0' && *s != sep)
		{
			length++;
			s++;
		}
		bs->length = length;
		bs->size = length;
		if (*s == sep)
			s++;
		num--;
	}
	
	return s;
}

bool UrlParam_Parse(const char* s, char sep, unsigned num, ...)
{
	va_list		ap;	
	
	va_start(ap, num);
	s = UrlParam_ParseVarArg(s, sep, num, ap);
	va_end(ap);
	
	if (num == 0 && *s == '\0')
		return true;
	else
		return false;
}

UrlParam::UrlParam()
{
	numParams = 0;
}

bool UrlParam::Init(const char* url_, char sep_)
{
	url = url_;
	sep = sep_;

	numParams = 0;
	buffer.Clear();
	params.Clear();
	
	return Parse();
}

int UrlParam::GetNumParams() const
{
	return numParams;
}

const char* UrlParam::GetParam(int nparam) const
{
	int offset;
	
	if (nparam >= numParams)
		return NULL;
	
	offset = *(int*)(params.buffer + nparam * sizeof(int) * 2);
	return &buffer.buffer[offset];
}

int UrlParam::GetParamLen(int nparam) const
{
	int length;
	
	if (nparam >= numParams)
		return NULL;
	
	length = *(int*)(params.buffer + nparam * sizeof(int) * 2 + sizeof(int));
	return length;
}

bool UrlParam::Get(unsigned num, ...) const
{
	va_list		ap;
	unsigned	i;
	ByteString* bs;
	
	va_start(ap, num);
	
	for (i = 0; i < num; i++)
	{
		bs = va_arg(ap, ByteString*);
		bs->buffer = (char*) GetParam(i);
		bs->size = GetParamLen(i);
		bs->length = bs->size;
	}
	
	va_end(ap);
	
	return true;
}

bool UrlParam::Parse()
{
	size_t		len;
	const char*	s;
	const char*	start;
	
	len = strlen(url);
	buffer.Reallocate(len + 1, false);
	
	s = start = url;
	while (*s)
	{
		if (*s == sep)
		{
			AddParam(start, s - start);
			start = s + 1;
		}
		s++;
	}
	
	if (s - start > 0)
		AddParam(start, s - start);
	
	return true;
}

void UrlParam::AddParam(const char *s, int length)
{
	int		offset;
	int		i;
	int		unlen; // length of the unencoded string
	char	c;

	unlen = 0;
	offset = buffer.length;

	for (i = 0; i < length; /* i increased in loop */)
	{
		if (s[i] == '%')
		{
			c = DecodeChar(s + i);
			i += 3;
		}
		else 
		{
			c = s[i];
			i += 1;
		}

		unlen += 1;
		buffer.Append(&c, 1);
	}
	// terminate the string with zero in buffer
	buffer.Append("", 1);
	
	params.Append(&offset, sizeof(int));
	params.Append(&unlen, sizeof(int));
	numParams++;
	
}

char UrlParam::DecodeChar(const char* s)
{
	if (s[0] != '%')
		return s[0];
	
	return HexToChar(s[1], s[2]);
}

char UrlParam::HexToChar(char uhex, char lhex)
{
	return (char)(((unsigned char) HexdigitToChar(uhex) * 16) + HexdigitToChar(lhex));
}

char UrlParam::HexdigitToChar(char hexdigit)
{
	if (hexdigit >= '0' && hexdigit <= '9')
		return hexdigit - '0';
	if (hexdigit >= 'a' && hexdigit <= 'f')
		return hexdigit - 'a' + 10;
	if (hexdigit >= 'A' && hexdigit <= 'F')
		return hexdigit - 'A' + 10;
	
	return 0;
}
