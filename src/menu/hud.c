#include "hud.h"

#include "menu_rendering.h"
#include "../resource/material_cache.h"
#include "menu_common.h"
#include "../cutscene/cutscene_runner.h"
#include "../spell/spell_render.h"

#define SPELL_SLOT_LOCATION_X   232
#define SPELL_SLOT_LOCATION_Y   152

#define SPELL_SLOT_OFFSET       18

#define MANA_WIDTH_RATIO        1.0f

#define MANA_TO_SIZE(mana)  (int)((mana) * MANA_WIDTH_RATIO)

#define MANA_BAR_X              20
#define MANA_BAR_Y              214
#define MANA_BAR_HEIGHT         4

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

    int max_width = MANA_TO_SIZE(spell_exec_max_mana(&hud->player->spell_exec));
    int current_width = MANA_TO_SIZE(spell_exec_current_mana(&hud->player->spell_exec));
    int prev_width = MANA_TO_SIZE(spell_exec_prev_mana(&hud->player->spell_exec));

    rspq_block_run(solid_primitive_material->block);
    rdpq_set_prim_color((color_t){255, 255, 255, 128});
    rdpq_texture_rectangle(
        TILE0, 
        MANA_BAR_X - 1, MANA_BAR_Y - 1, 
        MANA_BAR_X + max_width + 1, MANA_BAR_Y + MANA_BAR_HEIGHT + 1, 
        0, 0
    );
    rdpq_set_prim_color((color_t){80, 0, 240, 200});
    rdpq_texture_rectangle(
        TILE0, 
        MANA_BAR_X, MANA_BAR_Y, 
        MANA_BAR_X + current_width, MANA_BAR_Y + MANA_BAR_HEIGHT, 
        0, 0
    );
    if (current_width < prev_width) {
        rdpq_set_prim_color((color_t){220, 100, 0, 200});
        rdpq_texture_rectangle(
            TILE0, 
            MANA_BAR_X + current_width, MANA_BAR_Y, 
            MANA_BAR_X + prev_width, MANA_BAR_Y + MANA_BAR_HEIGHT, 
            0, 0
        );
    }

    spell_render(&hud->player->live_cast.pending_spell, 30, 30);
}

void hud_init(struct hud* hud, struct player* player) {
    menu_add_callback(hud_render, hud, 0);
    hud->player = player;
}

void hud_destroy(struct hud* hud) {
    menu_remove_callback(hud);
}