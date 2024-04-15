#include "cutscene_runner.h"

#include "../spell/spell.h"
#include "../time/time.h"
#include "../time/game_mode.h"
#include "../math/mathf.h"
#include "../menu/dialog_box.h"
#include "../menu/menu_rendering.h"
#include "../menu/menu_common.h"
#include "evaluation_context.h"
#include "expression_evaluate.h"
#include "show_rune.h"
#include <assert.h>

#define MAX_QUEUED_CUTSCENES   4
#define MAX_CUTSCENE_CALL_DEPTH 6

union cutscene_runner_data {
    struct { bool has_shown; } dialog;
};

struct cutscene_queue_entry {
    struct cutscene* cutscene;
    cutscene_finish_callback finish_callback;
    void* data;
};

struct cutscene_runner {
    uint16_t next_cutscene;
    uint16_t last_cutscene;

    struct cutscene_queue_entry queue[MAX_QUEUED_CUTSCENES];

    struct cutscene_queue_entry active_cutscenes[MAX_CUTSCENE_CALL_DEPTH];
    struct evaluation_context evaluation_context[MAX_CUTSCENE_CALL_DEPTH];
    uint16_t current_instruction[MAX_CUTSCENE_CALL_DEPTH];
    int16_t current_cutscene;
    int16_t current_context;

    struct show_rune show_rune;

    union cutscene_runner_data active_step_data;
};

static struct cutscene_runner cutscene_runner;

void cuscene_runner_start(struct cutscene* cutscene, cutscene_finish_callback finish_callback, void* data);

void cutscene_runner_init_step(struct cutscene_step* step) {
    switch (step->type)
    {
        case CUTSCENE_STEP_TYPE_DIALOG:
            cutscene_runner.active_step_data.dialog.has_shown = false;
            break;
        case CUTSCENE_STEP_TYPE_SHOW_RUNE:
            show_rune_start(&cutscene_runner.show_rune, &step->data);
            break;
        case CUTSCENE_STEP_TYPE_PAUSE:
            if (step->data.pause.should_pause) {
                if (step->data.pause.should_change_game_mode) {
                    current_game_mode = GAME_MODE_TRANSITION_TO_MENU;
                }
                update_pause_layers(UPDATE_LAYER_WORLD);
            } else {
                if (step->data.pause.should_change_game_mode) {
                    current_game_mode = GAME_MODE_3D;
                }
                update_unpause_layers(UPDATE_LAYER_WORLD);
            }
            break;
        case CUTSCENE_STEP_TYPE_PUSH_CONTEXT:
            cutscene_runner.current_context += 1;
            assert(cutscene_runner.current_context < MAX_CUTSCENE_CALL_DEPTH);
            evaluation_context_init(&cutscene_runner.evaluation_context[cutscene_runner.current_context], step->data.push_context.context_size);
            break;
        case CUTSCENE_STEP_TYPE_POP_CONTEXT:
            evaluation_context_destroy(&cutscene_runner.evaluation_context[cutscene_runner.current_context]);
            assert(cutscene_runner.current_context >= MAX_CUTSCENE_CALL_DEPTH);
            cutscene_runner.current_context -= 1;
            break;
        case CUTSCENE_STEP_TYPE_EXPRESSION:
            expression_evaluate(&cutscene_runner.evaluation_context[cutscene_runner.current_context], &step->data.expression.expression);
            break;
        case CUTSCENE_STEP_TYPE_JUMP_IF_NOT:
        case CUTSCENE_STEP_TYPE_JUMP:
            // logic is done in update step
            break;
        case CUTSCENE_STEP_TYPE_SET_LOCAL: {
            struct evaluation_context* context = &cutscene_runner.evaluation_context[cutscene_runner.current_context];

            evaluation_context_save(
                context->local_varaibles,
                step->data.store_variable.data_type,
                step->data.store_variable.word_offset,
                evaluation_context_pop(context)
            );
            break;
        }
        case CUTSCENE_STEP_TYPE_SET_GLOBAL: {
            struct evaluation_context* context = &cutscene_runner.evaluation_context[cutscene_runner.current_context];

            evaluation_context_save(
                context->global_variables,
                step->data.store_variable.data_type,
                step->data.store_variable.word_offset,
                evaluation_context_pop(context)
            );
            break;
        }
    }
}

bool cutscene_runner_update_step(struct cutscene_step* step) {
    switch (step->type)
    {
        case CUTSCENE_STEP_TYPE_DIALOG:
            if (!cutscene_runner.active_step_data.dialog.has_shown && !dialog_box_is_active()) {
                dialog_box_show(step->data.dialog.message.template, NULL, NULL);
                cutscene_runner.active_step_data.dialog.has_shown = true;
            } else if (cutscene_runner.active_step_data.dialog.has_shown) {
                return !dialog_box_is_active();
            }
            return false;
        case CUTSCENE_STEP_TYPE_SHOW_RUNE:
            return show_rune_update(&cutscene_runner.show_rune, &step->data);
        case CUTSCENE_STEP_TYPE_JUMP_IF_NOT:
            if (!evaluation_context_pop(&cutscene_runner.evaluation_context[cutscene_runner.current_context])) {
                cutscene_runner.current_instruction[cutscene_runner.current_cutscene] += step->data.jump.offset;
            }
            return true;
        case CUTSCENE_STEP_TYPE_JUMP:
            cutscene_runner.current_instruction[cutscene_runner.current_cutscene] += step->data.jump.offset;
            return true;
        default:
            return true;
    }
}

void cuscene_runner_start(struct cutscene* cutscene, cutscene_finish_callback finish_callback, void* data) {
    if (cutscene->step_count == 0) {
        return;
    }

    cutscene_runner.current_cutscene += 1;
    assert(cutscene_runner.current_cutscene < MAX_CUTSCENE_CALL_DEPTH);

    struct cutscene_queue_entry* next = &cutscene_runner.active_cutscenes[cutscene_runner.current_cutscene];
    next->cutscene = cutscene;
    next->finish_callback = finish_callback;
    next->data = data;
    cutscene_runner.current_instruction[cutscene_runner.current_cutscene] = 0;

    cutscene_runner_init_step(&cutscene->steps[0]);
}

void cutscene_runner_update(void* data) {
    if (!cutscene_runner_is_running()) {
        return;
    }

    struct cutscene_queue_entry* active_cutscene = &cutscene_runner.active_cutscenes[cutscene_runner.current_cutscene];

    struct cutscene_step* step = &active_cutscene->cutscene->steps[cutscene_runner.current_instruction[cutscene_runner.current_cutscene]];

    if (!cutscene_runner_update_step(step)) {
        return;
    }

    cutscene_runner.current_instruction[cutscene_runner.current_cutscene] += 1;

    while (cutscene_runner.current_cutscene >= 0) {
        step = &active_cutscene->cutscene->steps[cutscene_runner.current_instruction[cutscene_runner.current_cutscene]];
        active_cutscene = &cutscene_runner.active_cutscenes[cutscene_runner.current_cutscene];

        if (cutscene_runner.current_instruction[cutscene_runner.current_cutscene] < active_cutscene->cutscene->step_count) {
            cutscene_runner_init_step(step);
            return;
        }

        if (active_cutscene->finish_callback) {
            active_cutscene->finish_callback(active_cutscene->cutscene, active_cutscene->data);
        }

        cutscene_runner.current_cutscene -= 1;
    }

    struct cutscene_queue_entry* queue_entry = &cutscene_runner.queue[cutscene_runner.next_cutscene];

    if (!queue_entry->cutscene) {
        return;
    }

    cutscene_runner.next_cutscene += 1;

    if (cutscene_runner.next_cutscene == MAX_QUEUED_CUTSCENES) {
        cutscene_runner.next_cutscene = 0;
    }

    cuscene_runner_start(queue_entry->cutscene, queue_entry->finish_callback, queue_entry->data);
    queue_entry->cutscene = NULL;
}

void cutscene_runner_render(void* data) {
    show_rune_runder(&cutscene_runner.show_rune);
}

void cutscene_runner_init() {
    cutscene_runner.next_cutscene = 0;
    cutscene_runner.last_cutscene = 0;

    cutscene_runner.current_cutscene = -1;
    cutscene_runner.current_context = -1;

    show_init(&cutscene_runner.show_rune);
    update_add(&cutscene_runner, cutscene_runner_update, 0, UPDATE_LAYER_WORLD | UPDATE_LAYER_DIALOG | UPDATE_LAYER_PAUSE_MENU);
    menu_add_callback(cutscene_runner_render, &cutscene_runner, 0);

    for (int i = 0; i < MAX_QUEUED_CUTSCENES; i += 1) {
        cutscene_runner.queue[i].cutscene = NULL;
    }
}

void cutscene_runner_run(struct cutscene* cutscene, cutscene_finish_callback finish_callback, void* data) {
    if (cutscene_runner.current_cutscene == -1) {
        cuscene_runner_start(cutscene, finish_callback, data);
        return;
    }

    int next_cutscene = cutscene_runner.last_cutscene;

    cutscene_runner.last_cutscene += 1;

    if (cutscene_runner.last_cutscene == MAX_QUEUED_CUTSCENES) {
        cutscene_runner.last_cutscene = 0;
    }

    struct cutscene_queue_entry* queue_entry = &cutscene_runner.queue[next_cutscene];

    assert(!queue_entry->cutscene);

    queue_entry->cutscene = cutscene;
    queue_entry->finish_callback = finish_callback;
    queue_entry->data = data;
}

bool cutscene_runner_is_running() {
    return cutscene_runner.current_cutscene != -1;
}

void cutscene_runner_free_on_finish(struct cutscene* cutscene, void* data) {
    cutscene_free(cutscene);
}