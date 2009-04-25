#ifndef LOG_H
#define LOG_H

#include <string.h>
#include <errno.h>
#include <stdio.h>

#define LOG_TYPE_ERRNO		0
#define LOG_TYPE_MSG		1
#define LOG_TYPE_TRACE		2

#define Log_Errno() Log(__FILE__, __LINE__, __func__, LOG_TYPE_ERRNO, "")
#define Log_Message(...) Log(__FILE__, __LINE__, __func__, LOG_TYPE_MSG, __VA_ARGS__)
#define Log_Trace(...) Log(__FILE__, __LINE__, __func__, LOG_TYPE_TRACE, "" __VA_ARGS__)

#ifdef GCC
#define ATTRIBUTE_FORMAT_PRINTF(fmt, ellipsis) __attribute__ ((format (printf, fmt, ellipsis)));
#else
#define ATTRIBUTE_FORMAT_PRINTF(fmt, ellipsis)
#endif

void Log(char* file, int line, const char* func, int type, const char* fmt, ...) ATTRIBUTE_FORMAT_PRINTF(5, 6);
void Log_SetTrace(bool trace);
void Log_SetTimestamping(bool ts);
void Log_SetMaxLine(int maxLine);

#endif
