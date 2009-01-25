#ifndef COMMON_H
#define COMMON_H

#include <string.h>

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

long strntol(char* buffer, int size, int* nread);

#endif