#include "live_cast_renderer.h"

#include "../resource/material_cache.h"
#include "../player/inventory.h"
#include "../spell/spell_render.h"
#include "../render/coloru8.h"

#define SPELL_SLOT_OFFSET       18

struct slot_offset {
    uint8_t x, y;
};

#define SPELL_SLOT_COUNT    4

#define SPELL_SLOT_LOCATION_X   232
#define SPELL_SLOT_LOCATION_Y   152

static struct slot_offset slot_offsets[] = {
    [SPELL_SYMBOL_FIRE] = { SPELL_SLOT_OFFSET * 2, SPELL_SLOT_OFFSET },
    [SPELL_SYMBOL_ICE] = { 0, SPELL_SLOT_OFFSET },
    [SPELL_SYMBOL_EARTH] = { SPELL_SLOT_OFFSET, SPELL_SLOT_OFFSET * 2 },
    [SPELL_SYMBOL_AIR] = { SPELL_SLOT_OFFSET, 0 },
};

static color_t spell_active_colors[] = {
    [ITEM_TYPE_NONE] = { 255, 255, 255, 255 },
    [SPELL_SYMBOL_FIRE] = { 240, 100, 10, 255 },
    [SPELL_SYMBOL_ICE] = { 10, 200, 240, 255 },
    [SPELL_SYMBOL_EARTH] = { 10, 200, 40, 255 },
    [SPELL_SYMBOL_AIR] = { 240, 240, 10, 255 },
};

static color_t inactive_color = { 255, 255, 255, 128 };
static color_t secondary_mixin = { 240, 240, 255, 200 };

static color_t background_color = { 49, 46, 200, 255 };

void spell_render_icon(enum inventory_item_type type, int x, int y) {
    int source_x = type == SPELL_SYMBOL_RECAST ? 216 : (type - 1) * 24;

    rdpq_texture_rectangle_scaled(
        TILE0,
        x, y,
        x + 24, y + 24,
        source_x, 0,
        source_x + 24, 24
    );

    // __rdpq_texture_rectangle_raw_fx()
}

void live_cast_renderer_init(live_cast_renderer_t* live_cast_renderer, live_cast_t* live_cast) {
    live_cast_renderer->live_cast = live_cast;
    live_cast_renderer->icon_background = material_cache_load("rom:/materials/menu/current_spell_icon.mat");
    live_cast_renderer->spell_icons = material_cache_load("rom:/materials/spell/symbols.mat");

    for (int i = 0; i < 4; i += 1) {
        live_cast_renderer->symbol_modifiers[i] = (struct symbol_modifier_parameters) {
            .alpha = 255,
            .x_offset = slot_offsets[i + SPELL_SYMBOL_FIRE].x,
            .y_offset = slot_offsets[i + SPELL_SYMBOL_FIRE].y,
        };
    }
}

void live_cast_renderer_destroy(live_cast_renderer_t* live_cast_renderer) {
    material_cache_release(live_cast_renderer->icon_background);
    material_cache_release(live_cast_renderer->spell_icons);
}

struct symbol_modifier_parameters live_cast_renderer_determine_target(rune_pattern_t rune, int rune_index) {
    if (rune.primary_rune == ITEM_TYPE_NONE) {
        return (struct symbol_modifier_parameters){
            .alpha = 255,
            .x_offset = slot_offsets[rune_index].x,
            .y_offset = slot_offsets[rune_index].y,
        };
    }
    if (rune.primary_rune == rune_index) {
        return (struct symbol_modifier_parameters){
            .alpha = 255,
            .x_offset = SPELL_SLOT_OFFSET,
            .y_offset = SPELL_SLOT_OFFSET,
        };
    }

    return (struct symbol_modifier_parameters){
        .alpha = 0,
        .x_offset = slot_offsets[rune_index].x,
        .y_offset = slot_offsets[rune_index].y,
    };
}

void live_cast_renderer_render(live_cast_renderer_t* live_cast_renderer) {
    rspq_block_run(live_cast_renderer->icon_background->block);
    
    rune_pattern_t rune = live_cast_get_current_rune(live_cast_renderer->live_cast);

    for (int i = SPELL_SYMBOL_FIRE; i <= SPELL_SYMBOL_AIR; i += 1) {
        live_cast_renderer->symbol_modifiers[i - SPELL_SYMBOL_FIRE] = live_cast_renderer_determine_target(rune, i);
    }

    for (int i = SPELL_SYMBOL_FIRE; i <= SPELL_SYMBOL_AIR; i += 1) {
        struct symbol_modifier_parameters parameters = live_cast_renderer->symbol_modifiers[i - SPELL_SYMBOL_FIRE];

        if (!parameters.alpha) {
            continue;
        }

        background_color.a = parameters.alpha;
        
        rdpq_sync_pipe();
        rdpq_set_prim_color(background_color);

        int x = SPELL_SLOT_LOCATION_X + parameters.x_offset;
        int y = SPELL_SLOT_LOCATION_Y + parameters.y_offset;

        rdpq_texture_rectangle(
            TILE0,
            x, y,
            x + 32, y + 32,
            0, 0
        );
    }
    
    rspq_block_run(live_cast_renderer->spell_icons->block);
    
    for (int i = SPELL_SYMBOL_FIRE; i <= SPELL_SYMBOL_AIR; i += 1) {
        struct symbol_modifier_parameters parameters = live_cast_renderer->symbol_modifiers[i - SPELL_SYMBOL_FIRE];

        rdpq_sync_pipe();
        if (rune.primary_rune == ITEM_TYPE_NONE) {
            int symbol_level = inventory_get_item_level(i);
            if (symbol_level > 0) {
                rdpq_set_prim_color(spell_active_colors[ITEM_TYPE_NONE]);
            } else {
                continue;
            }
        } else {
            if (i == rune.primary_rune || rune_pattern_has_secondary(rune, i)) {
                rdpq_set_prim_color(spell_active_colors[i]);
            } else if (rune_pattern_symbol_count(rune) < inventory_get_item_level(rune.primary_rune)) {
                rdpq_set_prim_color(spell_active_colors[ITEM_TYPE_NONE]);
            } else {
                continue;
            }
        }

        spell_render_icon(
            i, 
            SPELL_SLOT_LOCATION_X + 4 + parameters.x_offset, 
            SPELL_SLOT_LOCATION_Y + 4 + parameters.y_offset
        );
    }
}

void live_cast_renderer_update(live_cast_renderer_t* live_cast_renderer) {

}