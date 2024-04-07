#ifndef __CUTSCENE_CUTSCENE_H__
#define __CUTSCENE_CUTSCENE_H__

#include <stdint.h>
#include <stdbool.h>

enum cutscene_step_type {
    CUTSCENE_STEP_TYPE_DIALOG,
    CUTSCENE_STEP_TYPE_SHOW_RUNE,
    CUTSCENE_STEP_TYPE_PAUSE,
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

#endif