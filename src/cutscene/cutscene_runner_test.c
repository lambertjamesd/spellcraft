#include "./cutscene_runner.h"
#include "../test/framework_test.h"
#include "../time/time.h"

struct on_fn_end {
    struct test_context* t;
    int result[4];
    int result_count;
    bool complete;
};

typedef struct on_fn_end on_fn_end_t;

void test_cutscene_end(cutscene_t* cutscene, void* data, evaluation_context_t* context) {
    on_fn_end_t* expect = (on_fn_end_t*)data;

    int result[4];

    test_eqi(expect->t, expect->result_count, context->current_stack);
    evaluation_context_popn(context, result, expect->result_count);

    for (int i = 0; i < expect->result_count; i += 1) {
        test_eqi(expect->t, expect->result[i], result[i]);
    }

    expect->complete = true;
}

#define MAX_UPDATE_ITERATIONS       10

void test_cutscene_runner(struct test_context* t) {
    cutscene_t* cutscene = cutscene_load("rom:/scripts/test/fn_call_test.script");

    on_fn_end_t expect = (on_fn_end_t){
        .t = t,
        .result = {5},
        .result_count = 1,
        .complete = false,
    };

    cutscene_runner_run(cutscene, cutscene_find_function_index(cutscene, "test_return"), test_cutscene_end, &expect, 0);

    for (int i = 0; !expect.complete && i < MAX_UPDATE_ITERATIONS; i += 1) {
        update_dispatch();
    }

    assert(expect.complete);
}