#include "spell.h"

struct spell_symbol spell_get_symbol(struct spell* spell, int col, int row) {
    struct spell_symbol result;
    result.reserved = 0;
    result.type = SPELL_SYMBOL_BLANK;

    return result;
}