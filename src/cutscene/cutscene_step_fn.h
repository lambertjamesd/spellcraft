#ifndef __CUTSCENE_CUTSCENE_STEP_FN_H__
#define __CUTSCENE_CUTSCENE_STEP_FN_H__

#include "evaluation_context.h"
#include "cutscene.h"
#include "cutscene_runner_context.h"
#include <stdbool.h>

enum cutscene_step_fn_index {
    CUTSCENE_FN_SAY,
    CUTSCENE_FN_ASK,
    CUTSCENE_FN_PAUSE,
    CUTSCENE_FN_UNPAUSE,
    CUTSCENE_FN_SHOW_ITEM,
    CUTSCENE_FN_DELAY,
    CUTSCENE_FN_INTERACT_NPC,
    CUTSCENE_FN_NPC_SET_SPEED,
    CUTSCENE_FN_INTERACT_POSITION,
    CUTSCENE_FN_NPC_WAIT,
    CUTSCENE_FN_CAMERA_WAIT,
    CUTSCENE_FN_CAMERA_FOLLOW,
    CUTSCENE_FN_CAMERA_RETURN,
    CUTSCENE_FN_CAMERA_LOOK_AT_NPC,
    CUTSCENE_FN_CAMERA_MOVE_TO,
    CUTSCENE_FN_CAMERA_LOOK_AT_POS,
    CUTSCENE_FN_LOAD_SCENE,
    CUTSCENE_FN_LOAD_FADE,
};

typedef void (*cutscene_step_fn_init)(cutscene_runner_context_t* context, int arg_count);
typedef bool (*cutscene_step_fn_step)(cutscene_runner_context_t* context);
typedef void (*cutscene_step_fn_cancel)(cutscene_runner_context_t* context);

struct cutscene_step_fn {
    cutscene_step_fn_init init;
    cutscene_step_fn_step step;
    cutscene_step_fn_cancel cancel;
};

typedef struct cutscene_step_fn cutscene_step_fn_t;

cutscene_step_fn_t* cutscene_step_lookup_fn(int type);

#endif