#include "cutscene_runner.h"

#include "../spell/spell.h"
#include "../time/time.h"
#include "../time/game_mode.h"
#include "../math/mathf.h"
#include "../menu/dialog_box.h"
#include "../menu/menu_rendering.h"
#include "../menu/menu_common.h"
#include "show_rune.h"
#include <assert.h>

#define MAX_QUEUED_CUTSCENE_STEPS   32

union cutscene_runner_data {
    struct { bool has_shown; } dialog;
};

struct cutscene_runner {
    struct cutscene_step steps[MAX_QUEUED_CUTSCENE_STEPS];
    uint16_t current_step;
    uint16_t max_step;

    bool is_active;

    struct show_rune show_rune;

    union cutscene_runner_data active_step_data;
};

static struct cutscene_runner cutscene_runner;

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
    }
}

bool cutscene_runner_update_step(struct cutscene_step* step) {
    switch (step->type)
    {
        case CUTSCENE_STEP_TYPE_DIALOG:
            if (!cutscene_runner.active_step_data.dialog.has_shown && !dialog_box_is_active()) {
                dialog_box_show(step->data.dialog.message, NULL, NULL);
                cutscene_runner.active_step_data.dialog.has_shown = true;
            } else if (cutscene_runner.active_step_data.dialog.has_shown) {
                return !dialog_box_is_active();
            }
            return false;
        case CUTSCENE_STEP_TYPE_SHOW_RUNE:
            return show_rune_update(&cutscene_runner.show_rune, &step->data);
        default:
            return true;
    }
}

void cutscene_runner_update(void* data) {
    if (!cutscene_runner_is_running()) {
        return;
    }

    if (!cutscene_runner_update_step(&cutscene_runner.steps[cutscene_runner.current_step])) {
        return;
    }

    cutscene_runner.current_step += 1;

    if (cutscene_runner.current_step == MAX_QUEUED_CUTSCENE_STEPS) {
        cutscene_runner.current_step = 0;
    }

    if (cutscene_runner.current_step == cutscene_runner.max_step) {
        cutscene_runner.is_active = false;
        return;
    }

    cutscene_runner_init_step(&cutscene_runner.steps[cutscene_runner.current_step]);
}

void cutscene_runner_render(void* data) {
    show_rune_runder(&cutscene_runner.show_rune);
}

void cutscene_runner_init() {
    cutscene_runner.current_step = 0;
    cutscene_runner.max_step = 0;
    show_init(&cutscene_runner.show_rune);
    cutscene_runner.is_active = false;

    update_add(&cutscene_runner, cutscene_runner_update, 0, UPDATE_LAYER_WORLD | UPDATE_LAYER_DIALOG | UPDATE_LAYER_PAUSE_MENU);
    menu_add_callback(cutscene_runner_render, &cutscene_runner, 0);
}

void cutscene_runner_queue_step(struct cutscene_step* step) {
    assert(!cutscene_runner.is_active || cutscene_runner.max_step != cutscene_runner.current_step);

    cutscene_runner.steps[cutscene_runner.max_step] = *step;

    cutscene_runner.max_step += 1;

    if (cutscene_runner.max_step == MAX_QUEUED_CUTSCENE_STEPS) {
        cutscene_runner.max_step = 0;
    }

    if (!cutscene_runner.is_active) {
        cutscene_runner_init_step(&cutscene_runner.steps[cutscene_runner.current_step]);
    }

    cutscene_runner.is_active = true;
}


void cutscene_runner_pause(bool should_pause, bool should_change_game_mode) {
    struct cutscene_step step = {
        .type = CUTSCENE_STEP_TYPE_PAUSE,
        .data = {
            .pause = {
                .should_pause = should_pause,
                .should_change_game_mode = should_change_game_mode,
            },
        },
    };

    cutscene_runner_queue_step(&step);
}

void cutscene_runner_dialog(char* message) {
    struct cutscene_step step = {
        .type = CUTSCENE_STEP_TYPE_DIALOG,
        .data = {
            .dialog = {
                .message = message,
            },
        },
    };

    cutscene_runner_queue_step(&step);
}

void cutscene_runner_show_rune(enum spell_symbol_type rune, bool should_show) {
    struct cutscene_step step = {
        .type = CUTSCENE_STEP_TYPE_SHOW_RUNE,
        .data = {
            .show_rune = {
                .rune = rune,
                .should_show = should_show,
            },
        },
    };

    cutscene_runner_queue_step(&step);
}

void cutscene_runner_run(struct cutscene* cutscene) {
    for (int i = 0; i < cutscene->step_count; i += 1) {
        cutscene_runner_queue_step(&cutscene->steps[i]);
    }
}

bool cutscene_runner_is_running() {
    return cutscene_runner.is_active;
}