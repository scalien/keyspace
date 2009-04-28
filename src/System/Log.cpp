#include "Log.h"
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#define ONLY_FILENAMES

#define LOG_MSG_SIZE	1024

static bool timestamping = false;
static bool trace = true;
static int	maxLine = LOG_MSG_SIZE;

static const char* GetFullTimestamp(char ts[27])
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

void Log_SetTrace(bool trace_)
{
	trace = trace_;
}

void Log_SetMaxLine(int maxLine_)
{
	maxLine = maxLine_ > LOG_MSG_SIZE ? LOG_MSG_SIZE : maxLine_;
}

void Log(const char* file, int line, const char* func, int type, const char* fmt, ...)
{
	int i, len;
	char buf[LOG_MSG_SIZE];
	char *msg;
	va_list ap;
	char ts[27];
	
	if (type == LOG_TYPE_TRACE && !trace)
		return;
	
	if (type == LOG_TYPE_ERRNO)
		msg = strerror(errno);
	else
	{
		va_start(ap, fmt);
		vsnprintf(buf, maxLine, fmt, ap);
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
		fprintf(stdout, "%s: ", GetFullTimestamp(ts));

//	if (type == LOG_TYPE_TRACE)
		fprintf(stdout, "%s:%d:%s()", file, line, func);

	if (msg[0] != '\0')
		fprintf(stdout, ": %s\n", msg);		
	else
		fprintf(stdout, "\n");
		
	fflush(stdout);
}
