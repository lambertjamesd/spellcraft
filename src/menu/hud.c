#include "hud.h"

#include "menu_rendering.h"
#include "../resource/material_cache.h"
#include "menu_common.h"
#include "../cutscene/cutscene_runner.h"
#include "../spell/spell_render.h"
#include "../spell/assets.h"
#include "../render/coloru8.h"

#define SPELL_SLOT_LOCATION_X   232
#define SPELL_SLOT_LOCATION_Y   152

#define SPELL_SLOT_OFFSET       18

#define MANA_WIDTH_RATIO        1.0f

#define MANA_TO_SIZE(mana)  (int)((mana) * MANA_WIDTH_RATIO)

#define MANA_BAR_X              20
#define MANA_BAR_Y              214
#define MANA_BAR_HEIGHT         4

#define HEALTH_BAR_Y            206

static color_t mana_color = {80, 0, 240, 200};
static color_t health_color = {240, 80, 0, 200};

void hud_draw_bar(int max_width, int current_width, int prev_width, int y, color_t color) {
    rspq_block_run(solid_primitive_material->block);
    rdpq_set_prim_color((color_t){255, 255, 255, 128});
    rdpq_texture_rectangle(
        TILE0, 
        MANA_BAR_X - 1, y - 1, 
        MANA_BAR_X + max_width + 1, y + MANA_BAR_HEIGHT + 1, 
        0, 0
    );
    rdpq_set_prim_color(color);
    rdpq_texture_rectangle(
        TILE0, 
        MANA_BAR_X, y, 
        MANA_BAR_X + current_width, y + MANA_BAR_HEIGHT, 
        0, 0
    );
    if (current_width < prev_width) {
        rdpq_set_prim_color((color_t){220, 100, 0, 200});
        rdpq_texture_rectangle(
            TILE0, 
            MANA_BAR_X + current_width, y, 
            MANA_BAR_X + prev_width, y + MANA_BAR_HEIGHT, 
            0, 0
        );
    }
}

void hud_render(void *data) {
    if (cutscene_runner_is_running()) {
        return;
    }
 
    struct hud* hud = (struct hud*)data;

    hud_draw_bar(
        MANA_TO_SIZE(spell_exec_max_mana(&hud->player->spell_exec)),
        MANA_TO_SIZE(spell_exec_current_mana(&hud->player->spell_exec)),
        MANA_TO_SIZE(spell_exec_prev_mana(&hud->player->spell_exec)),
        MANA_BAR_Y,
        mana_color
    );

    hud_draw_bar(
        MANA_TO_SIZE(hud->player->health.max_health),
        MANA_TO_SIZE(hud->player->health.current_health),
        MANA_TO_SIZE(0),
        HEALTH_BAR_Y,
        health_color
    );

    live_cast_renderer_render(&hud->live_cast_renderer);
}

void hud_init(struct hud* hud, struct player* player) {
    menu_add_callback(hud_render, hud, MENU_PRIORITY_HUD);
    hud->player = player;
    live_cast_renderer_init(&hud->live_cast_renderer, &player->live_cast);
}

void hud_destroy(struct hud* hud) {
    menu_remove_callback(hud);
    live_cast_renderer_destroy(&hud->live_cast_renderer);
}