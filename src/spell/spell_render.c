#include "spell_render.h"

#include <stdbool.h>
#include "assets.h"
#include "../time/time.h"
#include "../player/inventory.h"

#define MAX_MODIFIER_COUNT  4

#define MAX_BLOCKS  16

#define PRIMARY_WIDTH       28
#define MODIFIER_WIDTH      16
#define LAST_MODIFIER_WIDTH 20

#define SLIDE_IN_OFFSET     70
#define SLIDE_IN_TIME       0.25f

struct spell_block {
    uint16_t x_offset;
    uint8_t modfiier_count;
    uint8_t primary_rune;
    uint8_t modifier_runes[MAX_MODIFIER_COUNT];  
};

int spell_block_layout(struct spell_block* spell_blocks, struct spell* spell) {
    int x = 0;

    struct spell_block* current_block = spell_blocks;
    current_block->x_offset = x;
    current_block->modfiier_count = 0;
    current_block->primary_rune = 0;

    for (int col = 0; col < spell->cols; col += 1) {
        int type = spell->symbols[col].type;
        int next_type = col + 1 < spell->cols ? spell->symbols[col + 1].type : 0;

        if (type == 0) {
            break;
        }

        if (spell_is_rune(type)) {
            if (current_block->primary_rune) {
                if (current_block->modfiier_count < MAX_MODIFIER_COUNT) {
                    current_block->modifier_runes[current_block->modfiier_count] = type;
                    ++current_block->modfiier_count;
                }
            } else {
                current_block->primary_rune = type;
            }
        } else if (type == SPELL_SYMBOL_BREAK) {
            if (current_block->primary_rune == 0) {
                current_block->primary_rune = SPELL_SYMBOL_RECAST;
            }

            x += PRIMARY_WIDTH;
        
            if (current_block->modfiier_count) {
                x += current_block->modfiier_count * MODIFIER_WIDTH + (LAST_MODIFIER_WIDTH - MODIFIER_WIDTH);
            }

            ++current_block;

            current_block->x_offset = x;
            current_block->modfiier_count = 0;
            current_block->primary_rune = 0;
        }
    }

    // show available space for last rune block
    for (int i = current_block->modfiier_count; i < MAX_MODIFIER_COUNT; i += 1) {
        current_block->modifier_runes[i] = 0;
    }

    current_block->modfiier_count = MAX_MODIFIER_COUNT;

    if (current_block->primary_rune) {
        current_block->modfiier_count = inventory_get_item_level(current_block->primary_rune) - 1;
    }

    ++current_block;

    return current_block - spell_blocks;
}

int spell_render_offset(struct spell_render_animation* animation) {
    float time_since_symbol = total_time - animation->last_symbol_time;

    if (time_since_symbol < SLIDE_IN_TIME) {
        return -(int)(SLIDE_IN_OFFSET * (SLIDE_IN_TIME - time_since_symbol) * (1.0f / SLIDE_IN_TIME));
    }

    return 0;
}

void spell_render_border(struct spell_block* spell_blocks, int block_count, int left, int top, struct spell_render_animation* animation) {
    rspq_block_run(spell_assets_get()->casting_border->block);
    rdpq_set_prim_color((color_t){.r = 255, .g = 255, .b = 255, .a = 255});

    for (int index = 0; index < block_count; index += 1) {
        struct spell_block* current_block = &spell_blocks[index];
        int x = current_block->x_offset + left;

        if (index + 1 == block_count && (current_block->primary_rune == 0 || index == 0) && current_block->modifier_runes[0] == 0 && animation) {
            top += spell_render_offset(animation);
        }
        
        rdpq_texture_rectangle_scaled(
            TILE0,
            x, top,
            x + PRIMARY_WIDTH, top + 32,
            0, 0,
            PRIMARY_WIDTH, 32
        );
        x += PRIMARY_WIDTH;

        if (!current_block->modfiier_count) {
            continue;
        }

        for (int i = 0; i < current_block->modfiier_count - 1; i += 1) {
            rdpq_texture_rectangle_scaled(
                TILE0,
                x, top,
                x + MODIFIER_WIDTH, top + 32,
                PRIMARY_WIDTH, 0,
                PRIMARY_WIDTH + MODIFIER_WIDTH, 32
            );
            x += MODIFIER_WIDTH;
        }

        rdpq_texture_rectangle_scaled(
            TILE0,
            x, top,
            x + LAST_MODIFIER_WIDTH, top + 32,
            PRIMARY_WIDTH + MODIFIER_WIDTH, 0,
            PRIMARY_WIDTH + MODIFIER_WIDTH + LAST_MODIFIER_WIDTH, 32
        );
        x += LAST_MODIFIER_WIDTH;
    }
}

void spell_render(struct spell* spell, int left, int top, struct spell_render_animation* animation) {
    struct spell_block spell_blocks[MAX_BLOCKS];
    int block_count = spell_block_layout(spell_blocks, spell);

    spell_render_border(spell_blocks, block_count, left, top, animation);

    rspq_block_run(spell_assets_get()->spell_symbols->block);

    for (int index = 0; index < block_count; index += 1) {
        struct spell_block* current_block = &spell_blocks[index];
        int x = current_block->x_offset + left;

        if (!current_block->primary_rune) {
            continue;
        }

        int final_top = top;

        if (index + 1 == block_count && current_block->modifier_runes[0] == 0 && animation) {
            final_top += spell_render_offset(animation);
        }

        int source_x = current_block->primary_rune == SPELL_SYMBOL_RECAST ? 216 : (current_block->primary_rune - 1) * 24;

        rdpq_texture_rectangle_scaled(
            TILE0,
            x + 2, final_top + 4,
            x + 2 + 24, final_top + 4 + 24,
            source_x, 0,
            source_x + 24, 24
        );
    }

    rdpq_sync_pipe();
    rdpq_set_prim_color((color_t){.r = 0, .g = 0, .b = 0, .a = 255});

    for (int index = 0; index < block_count; index += 1) {
        struct spell_block* current_block = &spell_blocks[index];
        int x = current_block->x_offset + left + PRIMARY_WIDTH;

        for (int modifier = 0; modifier < current_block->modfiier_count; modifier += 1) {
            int type = current_block->modifier_runes[modifier];

            if (type == 0) {
                break;
            }

            if (index + 1 == block_count && 
                (modifier + 1 == current_block->modfiier_count || !current_block->modifier_runes[modifier + 1]) && 
                animation) {
                top += spell_render_offset(animation);
            }

            int source_x = 120 + MODIFIER_WIDTH * (type - 1);
            rdpq_texture_rectangle_scaled(
                TILE0,
                x, top + 12,
                x + 16, top + 12 + 16,
                source_x, 0,
                source_x + MODIFIER_WIDTH, MODIFIER_WIDTH
            );
            x += MODIFIER_WIDTH;
        }
    }
}