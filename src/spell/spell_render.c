#include "spell_render.h"

#include <stdbool.h>
#include "assets.h"

void spell_render_border(struct spell* spell, int left, int top) {
    rspq_block_run(spell_assets_get()->casting_border->block);
    rdpq_set_prim_color((color_t){.r = 255, .g = 255, .b = 255, .a = 255});

    int x = left;
    int block_offset = 0;

    for (int col = 0; col < spell->cols; col += 1) {
        int type = spell->symbols[col].type;
        int next_type = col + 1 < spell->cols ? spell->symbols[col + 1].type : 0;

        if (type == 0) {
            break;
        }

        if (type == SPELL_SYMBOL_BREAK) {
            block_offset = 0;
            continue;
        }

        if (block_offset == 0) {
            rdpq_texture_rectangle_scaled(
                TILE0,
                x, top,
                x + 28, top + 32,
                0, 0,
                28, 32
            );
            x += 28;
        } else if (next_type != SPELL_SYMBOL_BREAK && next_type != 0) {
            rdpq_texture_rectangle_scaled(
                TILE0,
                x, top,
                x + 16, top + 32,
                28, 0,
                44, 32
            );
            x += 16;
        } else {
            rdpq_texture_rectangle_scaled(
                TILE0,
                x, top,
                x + 20, top + 32,
                44, 0,
                64, 32
            );
            x += 20;
        }

        block_offset += 1;
    }

    if (block_offset == 1) {
        rdpq_texture_rectangle_scaled(
            TILE0,
            x, top,
            x + 20, top + 32,
            44, 0,
            64, 32
        );
        x += 20;
    }
}

void spell_render(struct spell* spell, int left, int top) {
    spell_render_border(spell, left, top);

    rspq_block_run(spell_assets_get()->spell_symbols->block);

    int x = left + 4;
    bool is_modifier = false;

    for (int col = 0; col < spell->cols; col += 1) {
        int type = spell->symbols[col].type;

        if (type == 0) {
            break;
        }

        if (type == SPELL_SYMBOL_BREAK) {
            is_modifier = false;

            rdpq_sync_pipe();
            rdpq_set_prim_color((color_t){.r = 255, .g = 255, .b = 255, .a = 255});

            x += 8;
            continue;
        }

        int size = is_modifier ? 16 : 24;

        int source_x = (type - 1) * size + (is_modifier ? 120 : 0);
        int symbol_left = is_modifier ? x : x - 1;
        int symbol_top = is_modifier ? top + 12 : top + 4;

        rdpq_texture_rectangle_scaled(
            TILE0,
            symbol_left, symbol_top,
            symbol_left + size, symbol_top + size,
            source_x, 0,
            source_x + size, size
        );

        x += size;

        if (!is_modifier) {
            rdpq_sync_pipe();
            rdpq_set_prim_color((color_t){.r = 0, .g = 0, .b = 0, .a = 255});
            is_modifier = true;
        }
    }
}