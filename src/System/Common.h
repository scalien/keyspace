#ifndef COMMON_H
#define COMMON_H

#include "assert.h"
#include <string.h>
#include "Types.h"
#include <math.h>

#define KB 1000
#define MB 1000000

#define MEMCMP(b1, l1, b2, l2) ((l1) == (l2) && memcmp((b1), (b2), l1) == 0)

#define SIZE(a) (sizeof((a)) / sizeof((a[0])))

inline int min(int a, int b)
{
	if (a < b) return a; else return b;
}

inline int max(int a, int b)
{
	if (a > b) return a; else return b;
}

//long strntol(char* buffer, int size, int* nread);

long64 strntolong64(char* buffer, int length, int* nread);

ulong64 strntoulong64(char* buffer, int length, int* nread);

char* rprintf(const char* format, ...);

void* Alloc(int num, int size = 1);

#define ASSERT_FAIL() assert(false)

inline bool Xor(bool a, bool b, bool c) { return (((int)a + (int)b + (int)c) == 1); }

inline bool Xor(bool a, bool b) { return Xor(a, b, false); }

inline unsigned NumLen(int n) { return (unsigned) floor(log10(n) + 1); }

#endif
