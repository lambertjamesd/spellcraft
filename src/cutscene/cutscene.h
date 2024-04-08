#ifndef __CUTSCENE_CUTSCENE_H__
#define __CUTSCENE_CUTSCENE_H__

#include <stdint.h>
#include <stdbool.h>
#include "../scene/world_definition.h"

enum cutscene_step_type {
    CUTSCENE_STEP_TYPE_DIALOG,
    CUTSCENE_STEP_TYPE_SHOW_RUNE,
    CUTSCENE_STEP_TYPE_PAUSE,
    CUTSCENE_STEP_TYPE_PUSH_CONTEXT,
    CUTSCENE_STEP_TYPE_POP_CONTEXT,
};

union cutscene_step_data {
    struct {
        char* message;
    } dialog;
    struct {
        uint16_t rune;
        uint16_t should_show;
    } show_rune;
    struct {
        bool should_pause: 1;
        bool should_change_game_mode: 1;
    } pause;
};

struct cutscene_step {
    enum cutscene_step_type type;
    union cutscene_step_data data;
};

struct cutscene {
    struct cutscene_step* steps;
    uint16_t step_count;
};

struct cutscene* cutscene_new(int capacity);
void cutscene_free(struct cutscene* cutscene);

#define MAX_BUILDER_STEP_COUNT  32

struct cutscene_builder {
    struct cutscene_step steps[MAX_BUILDER_STEP_COUNT];
    uint16_t step_count;
};

void cutscene_builder_init(struct cutscene_builder* builder);

void cutscene_builder_pause(struct cutscene_builder* builder, bool should_pause, bool should_change_game_mode);
void cutscene_builder_dialog(struct cutscene_builder* builder, char* message);
void cutscene_builder_show_rune(struct cutscene_builder* builder, enum spell_symbol_type rune, bool should_show);

struct cutscene* cutscene_builder_finish(struct cutscene_builder* builder);

#endif