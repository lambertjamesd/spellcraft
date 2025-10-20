#ifndef __SPELL_SPELL_H__
#define __SPELL_SPELL_H__

#include <stdint.h>
#include <stdbool.h>

#include "../scene/scene_definition.h"

#define SPELL_MAX_COLS    10
#define SPELL_MAX_ROWS    4

enum spell_icon {
    SPELL_ICON_FIRE,
    SPELL_ICON_DASH,

    SPELL_ICON_CUSTOM_0,
    SPELL_ICON_CUSTOM_1,
    SPELL_ICON_CUSTOM_2,
    SPELL_ICON_CUSTOM_3,
    SPELL_ICON_CUSTOM_4,
    SPELL_ICON_CUSTOM_5,
    
    SPELL_ICON_COUNT,
};

struct rune_pattern {
    uint8_t primary_rune: 3;
    uint8_t flaming: 1;
    uint8_t icy: 1;
    uint8_t windy: 1;
    uint8_t living: 1;
    uint8_t earthy: 1;
};

typedef struct rune_pattern rune_pattern_t;

#define MAX_SPELL_LENGTH 6

struct spell {
    rune_pattern_t symbols[MAX_SPELL_LENGTH];
    uint8_t length;
    uint8_t symbol_index;
};

typedef struct spell spell_t;

void spell_init(spell_t* spell, int icon);

rune_pattern_t spell_get_rune_pattern(spell_t* spell, int index);
void spell_set_rune_pattern(spell_t* spell, int index, rune_pattern_t rune);

void spell_append(spell_t* spell, rune_pattern_t value);

int spell_get_length(spell_t* spell);

bool rune_pattern_has_secondary(rune_pattern_t pattern, enum inventory_item_type symbol_type);
int rune_pattern_symbol_count(rune_pattern_t pattern);

#endif