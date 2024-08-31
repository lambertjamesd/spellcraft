#ifndef __CUTSCENE_CUTSCENE_H__
#define __CUTSCENE_CUTSCENE_H__

#include <stdint.h>
#include <stdbool.h>
#include "../scene/scene_definition.h"
#include "cutscene_actor.h"
#include "expression.h"

enum cutscene_step_type {
    CUTSCENE_STEP_DIALOG,
    CUTSCENE_STEP_SHOW_ITEM,
    CUTSCENE_STEP_PAUSE,
    CUTSCENE_STEP_EXPRESSION,
    CUTSCENE_STEP_JUMP_IF_NOT,
    CUTSCENE_STEP_JUMP,
    CUTSCENE_STEP_SET_LOCAL,
    CUTSCENE_STEP_SET_GLOBAL,
    CUTSCENE_STEP_DELAY,
    CUTSCENE_STEP_LOOK_AT_NPC,
    CUTSCENE_STEP_MOVE_TO_NPC,
    CUTSCENE_STEP_IDLE_NPC,
};

struct cutscene_step;

struct cutscene {
    struct cutscene_step* steps;
    uint16_t step_count;
    
    uint16_t locals_size;
    void* locals;
};

struct templated_string {
    char* template;
    uint16_t nargs;
};

union cutscene_step_data {
    struct {
        struct templated_string message;
    } dialog;
    struct {
        uint16_t item;
        uint16_t should_show;
    } show_item;
    struct {
        uint8_t should_pause;
        uint8_t should_change_game_mode;
        uint16_t layers;
    } pause;
    struct {
        uint16_t context_size;
    } push_context;
    struct {
        struct expression expression;
    } expression;
    struct {
        int offset;
    } jump;
    struct {
        uint16_t data_type;
        uint16_t word_offset;
    } store_variable;
    struct {
        float duration;
    } delay;
    struct {
        union cutscene_actor_id subject;
        union cutscene_actor_id target;
        float speed;
    } interact_with_npc;
};

struct cutscene_step {
    enum cutscene_step_type type;
    union cutscene_step_data data;
};

struct cutscene* cutscene_new(int capacity, int locals_capacity);
void cutscene_free(struct cutscene* cutscene);

#define MAX_BUILDER_STEP_COUNT  32

struct cutscene_builder {
    struct cutscene_step steps[MAX_BUILDER_STEP_COUNT];
    uint16_t step_count;
};

struct cutscene* cutscene_load(char* filename);
void cutscene_builder_init(struct cutscene_builder* builder);

void cutscene_builder_pause(struct cutscene_builder* builder, bool should_pause, bool should_change_game_mode, int layers);
void cutscene_builder_dialog(struct cutscene_builder* builder, char* message);
void cutscene_builder_show_item(struct cutscene_builder* builder, enum inventory_item_type item, bool should_show);
void cutscene_builder_delay(struct cutscene_builder* builder, float delay);

struct cutscene* cutscene_builder_finish(struct cutscene_builder* builder);

#endif