#include "Log.h"
#include <stdarg.h>

#define ONLY_FILENAMES

#define LOG_MSG_SIZE	1024

void Log(char* file, int line, const char* func, int type, const char* fmt, ...)
{
	int i, len;
	char buf[LOG_MSG_SIZE];
	char *msg;
	va_list ap;
	
	if (type == LOG_TYPE_ERRNO)
	{
		msg = strerror(errno);
	}
	else
	{
		va_start(ap, fmt);
		vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);
		msg = buf;
	}

#ifdef ONLY_FILENAMES	
	len = strlen(file);
	for (i = len - 1; i >= 0; i--)
		if (file[i] == '/' || file[i] == '\\')
		{
			file = &file[i + 1];
			break;
		}
#endif
	
	if (msg[0] != '\0')
		fprintf(stdout, "%s:%d:%s(): %s\n", file, line, func, msg);
	else
		fprintf(stdout, "%s:%d:%s()\n", file, line, func);
		
	fflush(stdout);
}
