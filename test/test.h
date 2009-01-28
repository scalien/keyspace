/*
 * test.h -- basic unit test framework
 */
#ifndef __TEST_H
#define __TEST_H

#include <stdio.h>

#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

#ifdef __cplusplus
extern "C" {
#endif

// NOTE the token-paste operator (##) behaves specially with __VA_ARGS__ after commas
#define TEST_LOG(fmt, ...) {printf("%s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);}
#define TEST(testfn) test(testfn, #testfn)
#define TESTARG(testfn) (testfn),(#testfn)

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

typedef int (*testfn_t)(void);

int test(testfn_t testfn, const char *testname);
int test_iter(testfn_t testfn, const char *testname, unsigned long niter);
int test_time(testfn_t testfn, const char *testname);
int test_iter_time(testfn_t testfn, const char *testname, unsigned long niter);
int test_eval(const char *ident, int result);
int test_system(const char *cmdline);

#ifdef __cplusplus
}
#endif

#endif
