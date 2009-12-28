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

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag = 0;

  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    tmpres /= 10;  /*convert into microseconds*/
    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }

  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }

  return 0;
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

