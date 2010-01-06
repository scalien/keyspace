#include "Platform.h"

#ifdef WIN32
#include <time.h>
#include <windows.h>
#else
#include <sys/time.h>
#include <pwd.h>
#include <unistd.h>
#endif

bool ChangeUser(const char *user)
{
#ifdef WIN32
	// cannot really do on Windows
	return true;
#else
	if (user != NULL && *user && (getuid() == 0 || geteuid() == 0)) 
	{
		struct passwd *pw = getpwnam(user);
		if (!pw)
			return false;
		
		setuid(pw->pw_uid);
	}

	return true;
#endif
}

#ifdef WIN32

int gettimeofday (struct timeval *tv, void* tz)
{
  union {
    LONGLONG ns100; /*time since 1 Jan 1601 in 100ns units */
    FILETIME ft;
  } now;

  GetSystemTimeAsFileTime (&now.ft);
  tv->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL);
  tv->tv_sec = (long) ((now.ns100 - 116444736000000000LL) / 10000000LL);
  return (0);
} 

#endif

uint64_t GetMilliTimestamp()
{
	uint64_t now;
	struct timeval tv;
	
    gettimeofday(&tv, NULL);
	
	now = tv.tv_sec;
	now *= 1000;
	now += tv.tv_usec / 1000;

	return now;
}

uint64_t GetMicroTimestamp()
{
	uint64_t now;
	struct timeval tv;
	
    gettimeofday(&tv, NULL);
	
	now = tv.tv_sec;
	now *= 1000000;
	now += tv.tv_usec;
    
	return now;
}

