#ifndef __TEST_FRAMEWORK_TEST_H__
#define __TEST_FRAMEWORK_TEST_H__

#include <setjmp.h>
#include <stdbool.h>

// two macros ensures any macro passed will
// be expanded before being stringified
#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

struct test_context {
    jmp_buf jump;
};

typedef void (*test_callback)(struct test_context* context);

void test_run_raw(test_callback callback, const char* name);
#define test_run(fn) test_run_raw(fn, #fn)

void test_equali_raw(struct test_context* t, int expected, int actual, const char* location);
#define test_equali(t, expected, actual) test_equali_raw(t, expected, actual, __FILE__ ":" STRINGIZE(__LINE__))

void test_report_failures();

void test_load_world(const char* name);

#endif