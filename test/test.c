#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TEST_HEAD(testfn, testname) \
	{ \
		if (!testname) { \
			testname = "<NONAMETEST>"; \
		} \
		if (!testfn) { \
			printf("test: %s failed, no test function!\n", testname); \
			return 1; \
		} \
		printf("test: %s begin ------------------------\n", testname); \
	}

int
test(testfn_t testfn, const char *testname)
{
	int res;

	TEST_HEAD(testfn, testname);
	
	res = testfn();

	if (res == 0) {
		printf("test: %s succeeded. -------------------\n", testname);
		return 0;
	} else {
		printf("test: %s failed!!!!!!!!!!!!!!!!!!!!!!!!\n", testname);
		return 1;
	}
}

int
test_iter(testfn_t testfn, const char *testname, unsigned long niter)
{
	unsigned long i;
	int res = 0;

	TEST_HEAD(testfn, testname);

	for (i = 0; i < niter; i++) {
		res += testfn();
	}

	if (res == 0) {
		printf("test: %s succeeded.\n", testname);
		return 0;
	} else {
		printf("test: %s failed!\n", testname);
		return 1;
	}
}



int
test_time(testfn_t testfn, const char *testname)
{
	int res;
	time_t t0, t1;

	TEST_HEAD(testfn, testname);

	t0 = time(NULL);
	res = testfn();
	t1 = time(NULL);

	if (res == 0) {
		printf("test: %s succeeded (elapsed time = %d s).\n", testname, (int)(t1 - t0));
		return 0;
	} else {
		printf("test: %s failed (elapsed time = %d s)!\n", testname, (int)(t1 - t0));
		return 1;
	}
}

int
test_iter_time(testfn_t testfn, const char *testname, unsigned long niter)
{
	unsigned long i;
	int res = 0;
	time_t t0, t1;

	TEST_HEAD(testfn, testname);

	t0 = time(NULL);
	for (i = 0; i < niter; i++) {
		res += testfn();
	}
	t1 = time(NULL);

	if (res == 0) {
		printf("test: %s succeeded (elapsed time = %d s).\n", testname, (int)(t1 - t0));
		return 0;
	} else {
		printf("test: %s failed (elapsed time = %d s)!\n", testname, (int)(t1 - t0));
		return 1;
	}
}

int
test_eval(const char *ident, int result)
{
	printf("test: %s %s (%d)\n", ident, result ? "failed." : "succeeded.", result);

#ifdef _WIN32
#ifdef _CONSOLE
	system("pause");
#endif
#endif
	
	return result;
}

int
test_system(const char *cmdline)
{
	int res;

	printf("test_system: %s begin ----------------------\n", cmdline);
	
	res = system(cmdline);
	if (res == 0) {
		printf("test_system: %s succeeded. -------------------\n", cmdline);
	} else {
		printf("test_system: %s failed!!!!!!!!!!!!!!!!!!!!!!!!\n", cmdline);
	}
	
	return res;
}

int
test_names_parse(testfn_t *test_functions, char *test_names, int *names, int size)
{
	int i = 0;
	char *p = test_names;
	TEST_LOG("test_names = %s", test_names);
	while (*p) {
		names[i++] = p - test_names;
		char *comma = strstr(test_names, ",");
		if (comma) {
			*comma = '\0';
			p = comma + 1;
			while (*p && *p <= ' ') p++;
		} else
			break;
	}

	return 0;
}

