#ifndef __CUTSCENE_SHOW_RUNE_H__
#define __CUTSCENE_SHOW_RUNE_H__

#include <stdint.h>
#include <stdbool.h>
#include "cutscene.h"
#include "../render/material.h"

struct show_rune {
    struct material* spell_symbol_material;
    uint16_t showing_rune;
    uint16_t should_show;
    float show_rune_timer;
};

void show_init(struct show_rune* show_rune);
void show_rune_start(struct show_rune* show_rune, union cutscene_step_data* data);
bool show_rune_update(struct show_rune* show_rune, union cutscene_step_data* data);
void show_rune_runder(struct show_rune* show_rune);

#endif