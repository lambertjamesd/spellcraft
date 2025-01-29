#include "spell_render.h"

#include "assets.h"

void spell_render(struct spell* spell, int left, int top) {
    rspq_block_run(spell_assets_get()->spell_symbols->block);

    for (int col = 0; col < spell->cols; col += 1) {
        int x = left + col * 24 + 32;
        int type = spell->symbols[col].type;

        if (type == 0) {
            break;
        }

        if (type == SPELL_SYMBOL_BREAK) {
            continue;
        }

        int source_x = (type - 1) * 24;

        rdpq_texture_rectangle_scaled(
            TILE0,
            x, top,
            x + 24, top + 24,
            source_x, 0,
            source_x + 24, 24
        );
    }
}