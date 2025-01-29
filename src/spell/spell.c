#include "spell.h"

#include <malloc.h>

void spell_init(struct spell* spell, uint8_t cols, uint8_t rows, int icon) {
    int cell_count = cols * rows;

    spell->symbols = malloc(sizeof(struct spell_symbol) * cell_count);
    spell->rows = rows;
    spell->cols = cols;
    spell->symbol_index = icon;
    memset(spell->symbols, 0, sizeof(struct spell_symbol) * cell_count);

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

union spell_modifier_flags spell_get_modifiers(struct spell* spell, int col, int row) {
    union spell_modifier_flags result = { .all = 0 };
    do {
        ++col;
        struct spell_symbol symbol = spell_get_symbol(spell, col, row);

        switch (symbol.type) {
            case SPELL_SYMBOL_FIRE:
                result.flaming = 1;
                break;
            case SPELL_SYMBOL_ICE:
                result.icy = 1;
                break;
            case SPELL_SYMBOL_EARTH:
                result.earthy = 1;
                break;
            case SPELL_SYMBOL_AIR:
                result.windy = 1;
                break;
            case SPELL_SYMBOL_LIFE:
                result.living = 1;
                break;
            default:
                return result;
        };
    } while (col < spell->cols);

    return result;
}

void spell_set_symbol(struct spell* spell, int col, int row, struct spell_symbol value) {
    if (col >= spell->cols || row >= spell->rows || col < 0 || row < 0) {
        return;
    }

    spell->symbols[col + row * spell->cols] = value;
}

bool spell_has_primary_event(struct spell* spell, int col, int row) {
    if (spell_get_symbol(spell, col, row).type == SPELL_SYMBOL_BREAK) {
        return true;
    }

    do {
        ++col;
        if (spell_get_symbol(spell, col, row).type == SPELL_SYMBOL_BREAK) {
            return true;
        }
    } while (col < spell->cols);

    return false;
}

bool spell_has_secondary_event(struct spell* spell, int col, int row) {
    return false;
}

int spell_get_primary_event(struct spell* spell, int col, int row) {
    if (spell_get_symbol(spell, col, row).type == SPELL_SYMBOL_BREAK) {
        return col + 1;
    }

    do {
        ++col;
        if (spell_get_symbol(spell, col, row).type == SPELL_SYMBOL_BREAK) {
            return col + 1 < spell->cols ? col + 1 : -1;
        }
    } while (col < spell->cols);

    return -1;
}