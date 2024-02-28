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
    
    SPELL_SYBMOL_PASS_DOWN,
};

struct spell_symbol {
    uint8_t reserved: 4;
    uint8_t type: 4;
};

struct spell {
    struct spell_symbol* symbols;
    uint8_t cols;
    uint8_t rows;
};

struct spell_symbol spell_get_symbol(struct spell* spell, int col, int row);
bool spell_has_primary_event(struct spell* spell, int col, int row);
bool spell_has_secondary_event(struct spell* spell, int col, int row);

#endif