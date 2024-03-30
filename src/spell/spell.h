#ifndef __SPELL_SPELL_H__
#define __SPELL_SPELL_H__

#include <stdint.h>
#include <stdbool.h>

#define SPELL_MAX_COLS    10
#define SPELL_MAX_ROWS    4

enum spell_symbol_type {
    SPELL_SYMBOL_BLANK,
    SPELL_SYMBOL_FIRE,
    SPELL_SYMBOL_PROJECTILE,
    SPELL_SYMBOL_PUSH,
    SPELL_SYMBOL_RECAST,
    SPELL_SYMBOL_STICKY_CAST,
    
    SPELL_SYMBOL_PASS_DOWN,

    SPELL_SYBMOL_COUNT,
};

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

struct spell_symbol {
    uint8_t reserved: 4;
    uint8_t type: 4;
};

struct spell {
    struct spell_symbol* symbols;
    uint8_t cols;
    uint8_t rows;
    uint8_t symbol_index;
};

void spell_init(struct spell* spell, uint8_t cols, uint8_t rows, int icon);
void spell_destroy(struct spell* spell);

struct spell_symbol spell_get_symbol(struct spell* spell, int col, int row);

void spell_set_symbol(struct spell* spell, int col, int row, struct spell_symbol value);

bool spell_has_primary_event(struct spell* spell, int col, int row);
bool spell_has_secondary_event(struct spell* spell, int col, int row);
bool spell_is_modifier(struct spell* spell, int col, int row);

#endif