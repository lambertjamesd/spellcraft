#include "framework_test.h"

#include <stdio.h>
#include <string.h>
#include "../scene/world.h"
#include "../scene/world_loader.h"

#define MAX_NAMED_FAILS 16

static int total_test_count;
static int failed_test_count;

static const char* failed_test_names[MAX_NAMED_FAILS];

void test_run_raw(test_callback callback, const char* name) {
    struct test_context context;
    total_test_count += 1;
    if (setjmp(context.jump)) {
        fprintf(stderr, "TEST FAIL %s\n", name);

        if (failed_test_count < MAX_NAMED_FAILS) {
            failed_test_names[failed_test_count] = name;
            failed_test_count += 1;
        }

    } else {
        fprintf(stderr, "TEST START %s...\n", name);
        callback(&context);
        fprintf(stderr, "TEST PASS %s\n", name);
    }
}

void test_equali_raw(struct test_context* t, int expected, int actual, const char* location) {
    if (expected == actual) {
        return;
    }

    fprintf(stderr, "ASSERTION FAILED expected %d actual %d at %s\n", expected, actual, location);
    longjmp(t->jump, 1);
}

void test_report_failures() {
    fprintf(stderr, "%d/%d tests passed\n", total_test_count - failed_test_count, total_test_count);

    for (int i = 0; i < failed_test_count && i < MAX_NAMED_FAILS; i += 1) {
        fprintf(stderr, "    TEST FAIL %s\n", failed_test_names[i]);
    }

    if (failed_test_count > MAX_NAMED_FAILS) {
        fprintf(stderr, "    and %d more", failed_test_count - MAX_NAMED_FAILS);
    }
}

static const char* current_world;
static struct world* current_test_world;

void test_load_world(const char* name) {
    if (current_world && strcmp(name, current_world) == 0) {
        return;
    }

    current_world = name;

    world_release(current_test_world);
    current_test_world = world_load(name);
}