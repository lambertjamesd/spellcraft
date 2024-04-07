#include "hud.h"

#include "menu_rendering.h"
#include "../resource/material_cache.h"
#include "menu_common.h"
#include "../cutscene/cutscene_runner.h"

#define SPELL_SLOT_LOCATION_X   232
#define SPELL_SLOT_LOCATION_Y   152

#define SPELL_SLOT_OFFSET       18

void hud_render_spell_icon(struct spell* spell, int x, int y) {
    if (!spell) {
        return;
    }
    
    rspq_block_run(menu_spell_icons[spell->symbol_index]->block);

    rdpq_texture_rectangle(
        TILE0,
        x, y,
        x + 24, y + 24,
        0, 0
    );
}

void hud_render(void *data) {
    if (cutscene_runner_is_running()) {
        return;
    }
 
    struct hud* hud = (struct hud*)data;

    rspq_block_run(current_spell_icon->block);

    rdpq_texture_rectangle(
        TILE0,
        SPELL_SLOT_LOCATION_X, SPELL_SLOT_LOCATION_Y + SPELL_SLOT_OFFSET,
        SPELL_SLOT_LOCATION_X + 32, SPELL_SLOT_LOCATION_Y + SPELL_SLOT_OFFSET + 32,
        0, 0
    );

    rdpq_texture_rectangle(
        TILE0,
        SPELL_SLOT_LOCATION_X + SPELL_SLOT_OFFSET, SPELL_SLOT_LOCATION_Y,
        SPELL_SLOT_LOCATION_X + SPELL_SLOT_OFFSET + 32, SPELL_SLOT_LOCATION_Y + 32,
        0, 0
    );

    rdpq_texture_rectangle(
        TILE0,
        SPELL_SLOT_LOCATION_X + SPELL_SLOT_OFFSET * 2, SPELL_SLOT_LOCATION_Y + SPELL_SLOT_OFFSET,
        SPELL_SLOT_LOCATION_X + SPELL_SLOT_OFFSET * 2 + 32, SPELL_SLOT_LOCATION_Y + SPELL_SLOT_OFFSET + 32,
        0, 0
    );

    rdpq_texture_rectangle(
        TILE0,
        SPELL_SLOT_LOCATION_X + SPELL_SLOT_OFFSET, SPELL_SLOT_LOCATION_Y + SPELL_SLOT_OFFSET * 2,
        SPELL_SLOT_LOCATION_X + SPELL_SLOT_OFFSET + 32, SPELL_SLOT_LOCATION_Y + SPELL_SLOT_OFFSET * 2 + 32,
        0, 0
    );

    hud_render_spell_icon(
        inventory_get_equipped_spell(0), 
        SPELL_SLOT_LOCATION_X + SPELL_SLOT_OFFSET + 4,
        SPELL_SLOT_LOCATION_Y + 4
    );

    hud_render_spell_icon(
        inventory_get_equipped_spell(1), 
        SPELL_SLOT_LOCATION_X + SPELL_SLOT_OFFSET + 4,
        SPELL_SLOT_LOCATION_Y + SPELL_SLOT_OFFSET * 2 + 4
    );

    hud_render_spell_icon(
        inventory_get_equipped_spell(2), 
        SPELL_SLOT_LOCATION_X + 4,
        SPELL_SLOT_LOCATION_Y + SPELL_SLOT_OFFSET + 4
    );

    hud_render_spell_icon(
        inventory_get_equipped_spell(3), 
        SPELL_SLOT_LOCATION_X + SPELL_SLOT_OFFSET * 2 + 4,
        SPELL_SLOT_LOCATION_Y + SPELL_SLOT_OFFSET + 4
    );
}

void hud_init(struct hud* hud) {
    menu_add_callback(hud_render, hud, 0);
}

void hud_destroy(struct hud* hud) {
    
}