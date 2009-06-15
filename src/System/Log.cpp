#include "Log.h"
#define _XOPEN_SOURCE 600
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#define ONLY_FILENAMES

#define LOG_MSG_SIZE	1024

static bool		timestamping = false;
static bool		trace = true;
static int		maxLine = LOG_MSG_SIZE;
static int		target = LOG_TARGET_STDOUT;
static FILE*	logfile = NULL;
static char*	logfilename = NULL;

typedef char log_timestamp_t[27];

static const char* GetFullTimestamp(log_timestamp_t ts)
{
	struct timeval tv;
	struct tm tm;
	time_t sec;

	if (!timestamping)
		return "";

	gettimeofday(&tv, NULL);
	sec = (time_t) tv.tv_sec;
	localtime_r(&sec, &tm);
	
	snprintf(ts, sizeof(log_timestamp_t), "%04d-%02d-%02d %02d:%02d:%02d.%06lu", 
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

void Log_SetTarget(int target_)
{
	target = target_;
}

bool Log_SetOutputFile(const char* filename)
{
	if (logfile)
	{
		fclose(logfile);
		free(logfilename);
		logfilename = NULL;
	}

	logfile = fopen(filename, "a");
	if (!logfile)
		return false;
	
	logfilename = strdup(filename);
	return true;
}

static void Log_Append(char*& p, int& remaining, const char* s, int len)
{
	if (len > remaining)
		len = remaining;
		
	if (len > 0)
		memcpy(p, s, len);
	
	p += len;
	remaining -= len;
}

static void Log_Flush(const char* buf, int /*size*/)
{
	if ((target & LOG_TARGET_STDOUT) == LOG_TARGET_STDOUT)
	{
		fputs(buf, stdout);
		fflush(stdout);
	}
	if ((target & LOG_TARGET_STDERR) == LOG_TARGET_STDERR)
	{
		fputs(buf, stderr);
		fflush(stderr);
	}
	if ((target & LOG_TARGET_FILE) == LOG_TARGET_FILE)
	{
		fputs(buf, logfile);
		fflush(logfile);
	}
}

void Log(const char* file, int line, const char* func, int type, const char* fmt, ...)
{
	char	buf[LOG_MSG_SIZE];
	int		remaining;
	char	*p;
	char	*sep;
	int		ret;
	va_list ap;
	
	if ((type == LOG_TYPE_TRACE || type == LOG_TYPE_ERRNO) && !trace)
		return;

	buf[maxLine - 1] = 0;
	p = buf;
	remaining = maxLine - 1;

	// print timestamp
	if (timestamping)
	{
		GetFullTimestamp(p);
		p += sizeof(log_timestamp_t) - 1;
		remaining -= sizeof(log_timestamp_t) - 1;
		Log_Append(p, remaining, ": ", 2);
	}

	if (file && func)
	{
		sep = strrchr(file, '/');
		if (sep)
			file = sep + 1;
		
		// print filename, number of line and function name
		ret = snprintf(p, remaining + 1, "%s:%d:%s()", file, line, func);
		if (ret < 0 || ret > remaining)
			ret = remaining;

		p += ret;
		remaining -= ret;

		if (fmt[0] != '\0' || type == LOG_TYPE_ERRNO)
			Log_Append(p, remaining, ": ", 2);
	}
	
	// in case of error print the errno message otherwise print our message
	if (type == LOG_TYPE_ERRNO)
	{

		// This is a workaround for g++ on Debian Lenny
		// see http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=485135
		// _GNU_SOURCE is unconditionally defined so always the GNU style
		// sterror_r() is used, which is broken
#ifdef _GNU_SOURCE
		char* err = strerror_r(errno, p, remaining - 1);
		if (err)
		{
				ret = strlen(err);
				if (err != p)
				{
						memcpy(p, err, ret);
						p[ret] = 0;
				}
		}
		else
				ret = -1;
#else
		ret = strerror_r(errno, p, remaining - 1);
#endif
		if (ret < 0)
			ret = remaining;

		p += ret;
		remaining -= ret;
	}
	else
	{
		va_start(ap, fmt);
		ret = vsnprintf(p, remaining + 1, fmt, ap);
		va_end(ap);
		if (ret < 0 || ret > remaining)
			ret = remaining;

		p += ret;
		remaining -= ret;
	}

	Log_Append(p, remaining, "\n", 2);
	Log_Flush(buf, maxLine - remaining);
}
