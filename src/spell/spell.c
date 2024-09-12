#include "spell.h"

#include <malloc.h>

void spell_init(struct spell* spell, uint8_t cols, uint8_t rows, int icon) {
    int cell_count = cols * rows;

    spell->symbols = malloc(sizeof(struct spell_symbol) * cell_count);
    spell->rows = rows;
    spell->cols = cols;
    spell->symbol_index = icon;

    struct spell_symbol* curr = spell->symbols;

    for (int i = 0; i < cell_count; i += 1) {
        curr->reserved = 0;
        curr->type = 0;
        ++curr;
    }
}

void spell_destroy(struct spell* spell) {
    free(spell->symbols);
    spell->symbols = 0;
}

struct spell_symbol spell_get_symbol(struct spell* spell, int col, int row) {
    if (col >= spell->cols || row >= spell->rows || col < 0 || row < 0) {
        struct spell_symbol result;
        result.reserved = 0;
        result.type = ITEM_TYPE_NONE;
        return result;
    }

    return spell->symbols[col + row * spell->cols];
}

void spell_set_symbol(struct spell* spell, int col, int row, struct spell_symbol value) {
    if (col >= spell->cols || row >= spell->rows || col < 0 || row < 0) {
        return;
    }

    spell->symbols[col + row * spell->cols] = value;
}

bool spell_has_primary_event(struct spell* spell, int col, int row) {
    return spell_get_symbol(spell, col + 1, row).type != ITEM_TYPE_NONE;
}

bool spell_has_secondary_event(struct spell* spell, int col, int row) {
    struct spell_symbol secondary_event = spell_get_symbol(spell, col + 1, row + 1);

    if (secondary_event.type == ITEM_TYPE_NONE) {
        return false;
    }

    struct spell_symbol sibling_symbol = spell_get_symbol(spell, col, row + 1);

    return sibling_symbol.type == ITEM_TYPE_NONE || sibling_symbol.type == SPELL_SYMBOL_PASS_DOWN;
}

static uint8_t is_modifier_mapping[] = {
    [ITEM_TYPE_NONE] = 0,
    [SPELL_SYMBOL_FIRE] = 1,
    [SPELL_SYMBOL_ICE] = 1,
    [SPELL_SYMBOL_EARTH] = 0,
    [SPELL_SYMBOL_AIR] = 1,
    [SPELL_SYMBOL_RECAST] = 0,
    [SPELL_SYMBOL_LIFE] = 0,
    
    [SPELL_SYMBOL_PASS_DOWN] = 0,
};

bool spell_is_modifier(struct spell* spell, int col, int row) {
    return is_modifier_mapping[spell_get_symbol(spell, col, row).type] && spell_has_primary_event(spell, col, row);
}