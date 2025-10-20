#include "spell.h"

#include <malloc.h>

void spell_init(spell_t* spell, int icon) {
    memset(spell, 0, sizeof(spell_t));
    spell->symbol_index = icon;
}

rune_pattern_t spell_get_rune_pattern(spell_t* spell, int index) {
    if (index >= spell->length) {
        return (rune_pattern_t){};
    }

    return spell->symbols[index];
}

void spell_set_rune_pattern(spell_t* spell, int index, rune_pattern_t rune) {
    if (index >= spell->length) {
        return;
    }

    spell->symbols[index] = rune;

    if (index == 0 && rune.primary_rune == ITEM_TYPE_NONE) {
        spell->length = 0;
    }
}

void spell_append(spell_t* spell, rune_pattern_t value) {
    if (spell->length < MAX_SPELL_LENGTH) {
        spell->symbols[spell->length] = value;
        ++spell->length;
    }
}
int spell_get_length(spell_t* spell) {
    int result = spell->length;

    while (result > 0 && spell->symbols[result].primary_rune != ITEM_TYPE_NONE) {
        --result;
    }

    return spell->length;
}

bool rune_pattern_has_secondary(rune_pattern_t pattern, enum inventory_item_type symbol_type) {
    switch (symbol_type)
    {
    case SPELL_SYMBOL_FIRE:
        return pattern.flaming;
    case SPELL_SYMBOL_ICE:
        return pattern.icy;
    case SPELL_SYMBOL_EARTH:
        return pattern.earthy;
    case SPELL_SYMBOL_AIR:
        return pattern.windy;
    case SPELL_SYMBOL_LIFE:
        return pattern.living;
    default:
        return false;
    }
}

int rune_pattern_symbol_count(rune_pattern_t pattern) {
    int result = 0;

    if (pattern.primary_rune != ITEM_TYPE_NONE) ++result;
    if (pattern.flaming) ++result;
    if (pattern.icy) ++result;
    if (pattern.earthy) ++result;
    if (pattern.windy) ++result;
    if (pattern.living) ++result;

    return result;
}