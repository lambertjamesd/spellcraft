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

void test_cutscene_end(cutscene_t* cutscene, void* data, cutscene_runner_context_t* context) {
    on_fn_end_t* expect = (on_fn_end_t*)data;

    int result[4];

    test_neqi(expect->t, 0, (int)context);
    test_eqi(expect->t, expect->result_count, context->current_stack);
    evaluation_context_popn(context, result, expect->result_count);

    for (int i = 0; i < expect->result_count; i += 1) {
        test_eqi(expect->t, expect->result[i], result[i]);
    }

    expect->complete = true;
}

#define MAX_UPDATE_ITERATIONS       10

void test_do_test(cutscene_t* cutscene, on_fn_end_t expect, const char* name) {
    cutscene_runner_run(cutscene, cutscene_find_function_index(cutscene, name), test_cutscene_end, &expect, 0);

    for (int i = 0; !expect.complete && i < MAX_UPDATE_ITERATIONS; i += 1) {
        update_dispatch();
    }

    assert(expect.complete);
}

void test_cutscene_runner(struct test_context* t) {
    cutscene_t* cutscene = cutscene_load("rom:/scripts/test/fn_call_test.script");

    test_do_test(
        cutscene, 
        (on_fn_end_t){
                .t = t,
                .result = {5},
                .result_count = 1,
                .complete = false,
        },
        "test_return"
    );
    
    test_do_test(
        cutscene, 
        (on_fn_end_t){
                .t = t,
                .result = {8},
                .result_count = 1,
                .complete = false,
        },
        "test_call_add"
    );
    
    test_do_test(
        cutscene, 
        (on_fn_end_t){
                .t = t,
                .result = {2, 3},
                .result_count = 2,
                .complete = false,
        },
        "multi_return"
    );
    
    test_do_test(
        cutscene, 
        (on_fn_end_t){
                .t = t,
                .result = {5, 3},
                .result_count = 2,
                .complete = false,
        },
        "local_swap"
    );
    
    test_do_test(
        cutscene, 
        (on_fn_end_t){
                .t = t,
                .result = {5},
                .result_count = 1,
                .complete = false,
        },
        "unpack_fn"
    );
}