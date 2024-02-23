#ifndef __SPELL_SPELL_H__
#define __SPELL_SPELL_H__

#include <stdint.h>

#define SPELL_MAX_COLS    10
#define SPELL_MAX_ROWS    4

enum spell_symbol_type {
    spell_symbol_type_blank,
    spell_symbol_type_fire,
    spell_symbol_type_ball,
    spell_symbol_type_push,
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

#endif