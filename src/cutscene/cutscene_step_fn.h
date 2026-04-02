#ifndef __CUTSCENE_CUTSCENE_STEP_FN_H__
#define __CUTSCENE_CUTSCENE_STEP_FN_H__

#include "evaluation_context.h"
#include "cutscene.h"
#include "cutscene_runner_context.h"
#include <stdbool.h>

typedef void (*cutscene_step_fn_init)(cutscene_runner_context_t* context, void* active_step_data, int arg_count);
typedef bool (*cutscene_step_fn_step)(cutscene_runner_context_t* context, void* active_step_data);

struct cutscene_step_fn {
    cutscene_step_fn_init init;
    cutscene_step_fn_step step;
};

typedef struct cutscene_step_fn cutscene_step_fn_t;

cutscene_step_fn_t* cutscene_step_lookup_fn(int type);

#endif