#ifndef __CUTSCENE_CUTSCENE_RUNNER_H__
#define __CUTSCENE_CUTSCENE_RUNNER_H__

#include "cutscene.h"
#include "../scene/scene_definition.h"
#include <stdbool.h>
#include "../entity/entity_id.h"
#include "evaluation_context.h"
#include "cutscene_runner_context.h"

typedef void (*cutscene_finish_callback)(struct cutscene* cutscene, void* data, cutscene_runner_context_t* context);

enum cutscene_channel {
    CUTSCENE_CHANNEL_BACKGROUND,
    CUTSCENE_CHANNEL_INTERACTION,
    CUTSCENE_CHANNEL_ANIMATION,

    CUTSCENE_CHANNEL_COUNT,
};

typedef enum cutscene_channel cutscene_channel_t;

struct cutscene_run_args {
    struct cutscene* cutscene;
    int16_t function_index;
    uint8_t channel;
    cutscene_finish_callback finish_callback;
    void* data;
    entity_id subject;
};

typedef struct cutscene_run_args cutscene_run_args_t;

void cutscene_runner_init();

void cutscene_runner_run(cutscene_run_args_t* args);
bool cutscene_runner_is_running(cutscene_channel_t channel);

void cutscene_runner_cancel(struct cutscene* cutscene);

// this is only a function that returns a callback to help balance out the pairing_checker
cutscene_finish_callback cutscene_runner_free_on_finish();


#endif