#include "live_cast_renderer.h"

#include "../resource/material_cache.h"
#include "../player/inventory.h"
#include "../spell/spell_render.h"
#include "../render/coloru8.h"

#define SPELL_SLOT_OFFSET       18

#define SPELL_SPACING           48

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

void spell_render_icon(enum inventory_item_type type, int x, int y, int size) {
    int source_x = type == SPELL_SYMBOL_RECAST ? 216 : (type - 1) * 24;

    rdpq_texture_rectangle_scaled(
        TILE0,
        x, y,
        x + size, y + size,
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
            .background_alpha =  255,
            .rune_alpha = 255,
            .x_offset = slot_offsets[i + SPELL_SYMBOL_FIRE].x,
            .y_offset = slot_offsets[i + SPELL_SYMBOL_FIRE].y,
            .size = 24,
        };
    }

    live_cast_renderer->prev_runes_offset = 0;
    live_cast_renderer->last_active_index = 0;
}

void live_cast_renderer_destroy(live_cast_renderer_t* live_cast_renderer) {
    material_cache_release(live_cast_renderer->icon_background);
    material_cache_release(live_cast_renderer->spell_icons);
}

struct symbol_modifier_parameters live_cast_renderer_determine_target(rune_pattern_t rune, int rune_index, bool has_room) {
    if (rune.primary_rune == ITEM_TYPE_NONE) {
        bool has_rune = inventory_get_item_level(rune_index) > 0;

        return (struct symbol_modifier_parameters){
            .background_alpha =  255,
            .rune_alpha = has_rune ? 255 : 0,
            .x_offset = slot_offsets[rune_index].x,
            .y_offset = slot_offsets[rune_index].y,
            .size = has_rune ? 24 : 16,
        };
    }
    if (rune.primary_rune == rune_index) {
        return (struct symbol_modifier_parameters){
            .background_alpha =  255,
            .rune_alpha = 255,
            .x_offset = SPELL_SLOT_OFFSET,
            .y_offset = SPELL_SLOT_OFFSET,
            .size = 24,
        };
    }

    return (struct symbol_modifier_parameters){
        .background_alpha =  0,
        .rune_alpha = has_room || rune_pattern_has_secondary(rune, rune_index) ? 255 : 0,
        .x_offset = slot_offsets[rune_index].x,
        .y_offset = slot_offsets[rune_index].y,
        .size = 16,
    };
}

int move_towards_int(int from, int to, int max_delta) {
    if (from > to) {
        from -= max_delta;
        
        if (from < to) {
            return to;
        }

        return from;
    }

    from += max_delta;

    if (from > to) {
        return to;
    }

    return from;
}

void live_cast_render_symbol_runes(rune_pattern_t rune, int x, int y) {
    if (!rune.primary_rune) {
        return;
    }

    rdpq_sync_pipe();
    rdpq_set_prim_color(spell_active_colors[rune.primary_rune]);

    spell_render_icon(
        rune.primary_rune, 
        x + SPELL_SLOT_OFFSET + 4, 
        y + SPELL_SLOT_OFFSET + 4,
        24
    );
    
    for (int i = SPELL_SYMBOL_FIRE; i <= SPELL_SYMBOL_AIR; i += 1) {
        if (!rune_pattern_has_secondary(rune, i)) {
            continue;
        }
        
        rdpq_sync_pipe();
        rdpq_set_prim_color(spell_active_colors[i]);

        spell_render_icon(
            i, 
            x + slot_offsets[i].x + 8, 
            y + slot_offsets[i].y + 8,
            16
        );
    }
}

void live_cast_renderer_render(live_cast_renderer_t* live_cast_renderer) {
    rspq_block_run(live_cast_renderer->icon_background->block);

    int prev_spell_count = live_cast_prev_rune_count(live_cast_renderer->live_cast);

    if (live_cast_renderer->last_active_index != prev_spell_count) {
        for (int i = 0; i < 4; i += 1) {
            live_cast_renderer->symbol_modifiers[i] = (struct symbol_modifier_parameters) {
                .background_alpha =  0,
                .rune_alpha = 0,
                .x_offset = slot_offsets[i + SPELL_SYMBOL_FIRE].x,
                .y_offset = slot_offsets[i + SPELL_SYMBOL_FIRE].y,
                .size = 24,
            };
        }

        live_cast_renderer->last_active_index = prev_spell_count;
    }
    
    rune_pattern_t rune = live_cast_get_current_rune(live_cast_renderer->live_cast);

    bool has_more = rune_pattern_symbol_count(rune) < inventory_get_item_level(rune.primary_rune);

    for (int i = SPELL_SYMBOL_FIRE; i <= SPELL_SYMBOL_AIR; i += 1) {
        struct symbol_modifier_parameters target = live_cast_renderer_determine_target(rune, i, has_more);
        struct symbol_modifier_parameters* parameters = &live_cast_renderer->symbol_modifiers[i - SPELL_SYMBOL_FIRE];
        
        parameters->background_alpha = move_towards_int(parameters->background_alpha, target.background_alpha, 64);
        parameters->rune_alpha = move_towards_int(parameters->rune_alpha, target.rune_alpha, 64);
        parameters->x_offset = move_towards_int(parameters->x_offset, target.x_offset, 4);
        parameters->y_offset = move_towards_int(parameters->y_offset, target.y_offset, 4);
        parameters->size = move_towards_int(parameters->size, target.size, 1);
    }
    
    live_cast_renderer->prev_runes_offset = move_towards_int(
        live_cast_renderer->prev_runes_offset,
        -SPELL_SPACING * prev_spell_count,
        SPELL_SPACING / 8
    );

    for (int i = SPELL_SYMBOL_FIRE; i <= SPELL_SYMBOL_AIR; i += 1) {
        struct symbol_modifier_parameters parameters = live_cast_renderer->symbol_modifiers[i - SPELL_SYMBOL_FIRE];

        if (!parameters.background_alpha) {
            continue;
        }

        background_color.a = parameters.background_alpha;
        
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

    if (prev_spell_count) {
        background_color.a = 255;
        rdpq_sync_pipe();
        rdpq_set_prim_color(background_color);
    }
    
    for (int i = 0; i < prev_spell_count; i += 1) {
        rune_pattern_t prev_rune = live_cast_get_rune(live_cast_renderer->live_cast, i);

        int x = SPELL_SLOT_LOCATION_X + SPELL_SLOT_OFFSET + i * SPELL_SPACING + live_cast_renderer->prev_runes_offset;
        int y = SPELL_SLOT_LOCATION_Y + SPELL_SLOT_OFFSET;

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

        color_t color = spell_active_colors[ITEM_TYPE_NONE];

        if (i == rune.primary_rune || rune_pattern_has_secondary(rune, i)) {
            color = spell_active_colors[i];
        }
        
        color.a = parameters.rune_alpha;

        if (color.a == 0) {
            continue;
        }
        
        rdpq_sync_pipe();
        rdpq_set_prim_color(color);

        spell_render_icon(
            i, 
            SPELL_SLOT_LOCATION_X + ((32 - parameters.size) >> 1) + parameters.x_offset, 
            SPELL_SLOT_LOCATION_Y + ((32 - parameters.size) >> 1) + parameters.y_offset,
            parameters.size
        );
    }

    for (int i = 0; i < prev_spell_count; i += 1) {
        rune_pattern_t prev_rune = live_cast_get_rune(live_cast_renderer->live_cast, i);

        int x = SPELL_SLOT_LOCATION_X + i * SPELL_SPACING + live_cast_renderer->prev_runes_offset;
        int y = SPELL_SLOT_LOCATION_Y;

        live_cast_render_symbol_runes(prev_rune, x, y);
    }
}

void live_cast_renderer_update(live_cast_renderer_t* live_cast_renderer) {

}