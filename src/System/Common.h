#ifndef COMMON_H
#define COMMON_H

#include "assert.h"
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "Log.h"

#define KB 1000
#define MB 1000000

#define MEMCMP(b1, l1, b2, l2) ((l1) == (l2) && memcmp((b1), (b2), l1) == 0)

#define SIZE(a) (sizeof((a)) / sizeof((a[0])))

#define ASSERT_FAIL() assert(false)

#define STOP_FAIL(msg, code) { Log_Message(msg); _exit(code); }

#define CS_INT_SIZE(int_type) ((size_t)(0.30103 * sizeof(int_type) * 8) + 2 + 1)

inline int min(int a, int b) { if (a < b) return a; else return b; }

inline int max(int a, int b) { if (a > b) return a; else return b; }

int64_t strntoint64_t(const char* buffer, int length, unsigned* nread);

uint64_t strntouint64_t(const char* buffer, int length, unsigned* nread);

const char* rprintf(const char* format, ...);

void* Alloc(int num, int size = 1);

inline bool Xor(bool a, bool b, bool c) { return (((int)a + (int)b + (int)c) == 1); }

inline bool Xor(bool a, bool b) { return Xor(a, b, false); }

inline unsigned NumLen(int n) { return n == 0 ? 1 : (unsigned) floor(log10(n) + 1); }

#endif
