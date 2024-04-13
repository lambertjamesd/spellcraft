#include "cutscene.h"

#include <malloc.h>
#include <memory.h>
#include <assert.h>

void cutscene_init(struct cutscene* cutscene, int capacity) {
    cutscene->steps = malloc(sizeof(struct cutscene_step) * capacity);
    cutscene->step_count = capacity;
}

void cutscene_destroy(struct cutscene* cutscene) {
    // TODO free any sub cutscenes
    free(cutscene->steps);
}

struct cutscene* cutscene_new(int capacity) {
    struct cutscene* result = malloc(sizeof(struct cutscene));
    cutscene_init(result, capacity);
    return result;
}

void cutscene_free(struct cutscene* cutscene) {
    cutscene_destroy(cutscene);
    free(cutscene);
}

void cutscene_builder_init(struct cutscene_builder* builder) {
    builder->step_count = 0;
}

struct cutscene_step* cutscene_builder_next_step(struct cutscene_builder* builder) {
    assert(builder->step_count < MAX_BUILDER_STEP_COUNT);

    struct cutscene_step* result = &builder->steps[builder->step_count];
    builder->step_count += 1;
    return result;
}

void cutscene_builder_pause(struct cutscene_builder* builder, bool should_pause, bool should_change_game_mode) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);

    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_TYPE_PAUSE,
        .data = {
            .pause = {
                .should_pause = should_pause,
                .should_change_game_mode = should_change_game_mode,
            },
        },
    };
}

void cutscene_builder_dialog(struct cutscene_builder* builder, char* message) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);

    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_TYPE_DIALOG,
        .data = {
            .dialog = {
                .message = {
                    .template = message,
                    .nargs = 0,
                },
            },
        },
    };
}

void cutscene_builder_show_rune(struct cutscene_builder* builder, enum spell_symbol_type rune, bool should_show) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);
    
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_TYPE_SHOW_RUNE,
        .data = {
            .show_rune = {
                .rune = rune,
                .should_show = should_show,
            },
        },
    };
}

struct cutscene* cutscene_builder_finish(struct cutscene_builder* builder) {
    struct cutscene* result = cutscene_new(builder->step_count);
    memcpy(result->steps, builder->steps, sizeof(struct cutscene_step) * builder->step_count);
    return result;
}
