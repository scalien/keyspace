#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
//#define _WIN32_WINNT			0x0400	// Windows NT 4.0
//#define WIN32_LEAN_AND_MEAN

#include <stddef.h>		// for intptr_t
#include <malloc.h>		// for _alloca()

#pragma warning(disable: 4244)	// conversion to smaller type, possible loss of data
#pragma warning(disable: 4267)	// conversion from size_t to smaller type, possible loss of data
#pragma warning(disable: 4355)	// 'this' : used in base member initializer list

// VC++ 8.0 is not C99-compatible
typedef __int8				int8_t;
typedef __int16				int16_t;
typedef __int32				int32_t;
typedef __int64				int64_t;

typedef unsigned __int8		uint8_t;
typedef unsigned __int16	uint16_t;
typedef unsigned __int32	uint32_t;
typedef unsigned __int64	uint64_t;

#define snprintf			_snprintf
#define strdup				_strdup

// 64bit compatible format string specifiers according to this document
// http://msdn.microsoft.com/en-us/library/tcxf1dw6.aspx
#define PRIu64				"I64i"
#define PRIi64				"I64i"

#define alloca				_alloca
#define alloca16(x)			((void *)((((intptr_t)alloca((x) + 15)) + 15) & ~15))

#define localtime_r(t, tm)	localtime_s(tm, t)


#else
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#endif

// PLATFORM_STRING selection
#ifdef PLATFORM_WINDOWS

#ifdef _WIN64
#define PLATFORM_STRING		"Windows 64bit"
#else
#define PLATFORM_STRING		"Windows"
#endif

#else

#ifdef PLATFORM_LINUX
#ifdef __amd64__
#define PLATFORM_STRING		"Linux 64bit"
#else
#define PLATFORM_STRING		"Linux"
#endif
#endif

#ifdef PLATFORM_DARWIN
#ifdef __amd64__
#define PLATFORM_STRING		"Darwin 64bit"
#else
#define PLATFORM_STRING		"Darwin"
#endif
#endif

#endif

bool		ChangeUser(const char *username);
uint64_t	GetMicroTimestamp();
uint64_t	GetMilliTimestamp();

#endif
