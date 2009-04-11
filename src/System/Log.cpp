#include "Log.h"
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#define ONLY_FILENAMES

#define LOG_MSG_SIZE	1024

static bool timestamping = false;

static char* GetFullTimestamp(char ts[27])
{
	struct timeval tv;
	struct tm tm;
	time_t sec;

	if (!timestamping)
		return "";

	gettimeofday(&tv, NULL);
	sec = (time_t) tv.tv_sec;
	localtime_r(&sec, &tm);
	
	snprintf(ts, 27, "%04d-%02d-%02d %02d:%02d:%02d.%06lu", 
			tm.tm_year + 1900,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec,
			(long unsigned int)tv.tv_usec);
	
	return ts;
}

void Log_SetTimestamping(bool ts)
{
	timestamping = ts;
}

void Log(char* file, int line, const char* func, int type, const char* fmt, ...)
{
	int i, len;
	char buf[LOG_MSG_SIZE];
	char *msg;
	va_list ap;
	char ts[27];
	
	if (type == LOG_TYPE_ERRNO)
		msg = strerror(errno);
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
	
	if (timestamping)
		fprintf(stdout, "%s:", GetFullTimestamp(ts));

	if (msg[0] != '\0')
		fprintf(stdout, "%s:%d:%s(): %s\n", file, line, func, msg);		
	else
		fprintf(stdout, "%s:%d:%s()\n", file, line, func);
		
	fflush(stdout);
}
