#include "hud.h"

#include "menu_rendering.h"
#include "../resource/material_cache.h"
#include "menu_common.h"
#include "../cutscene/cutscene_runner.h"
#include "../spell/spell_render.h"
#include "../spell/assets.h"
#include "../render/coloru8.h"
#include "../resource/material_cache.h"
#include "../time/time.h"
#include "../font/fonts.h"
#include "../render/defs.h"

#define SCREEN_EDGE_MARGIN      20.0f
#define TEXT_PADDING            2
#define BOX_HEIGHT              10

#define SPELL_SLOT_LOCATION_X   232
#define SPELL_SLOT_LOCATION_Y   152

#define SPELL_SLOT_OFFSET       18

#define MANA_WIDTH_RATIO        1.0f

#define MANA_TO_SIZE(mana)  (int)((mana) * MANA_WIDTH_RATIO)

#define MANA_BAR_X              24
#define MANA_BAR_Y              28
#define MANA_BAR_HEIGHT         4

#define HEALTH_BAR_Y            20

#define BUTTON_ICON_X           258
#define BUTTON_ICON_Y           30
#define BUTTON_ICON_SIZE        32

#define BOSS_HEALTH_WIDTH       260
#define BOSS_HEALTH_Y           20

static color_t mana_color = {80, 0, 240, 200};
static color_t health_color = {240, 80, 0, 200};

int measure_text(enum font_type font, const char* message) {
    const char* curr = message;

    int result = 0;

    while (*curr) {
        rdpq_font_gmetrics_t metrics;
        bool was_found = rdpq_font_get_glyph_metrics(font_get(font), *curr, &metrics);
        ++curr;

        if (was_found) {
            result += metrics.xadvance;
        }
    }
    
    return result;
}

void hud_render_interaction_preview(struct hud* hud) {
    if (!hud->player->hover_interaction || !update_has_layer(UPDATE_LAYER_WORLD)) {
        return;
    }

    interactable_t *target = interactable_get(hud->player->hover_interaction);

    if (!target) {
        return;
    }

    const char* interaction_name = interact_type_to_name(target->interact_type);

    if (!interaction_name) {
        return;
    }

    dynamic_object_t *obj = collision_scene_find_object(hud->player->hover_interaction);

    if (!obj) {
        return;
    }

    vector2_t screen_pos;
    vector3_t pos = *obj->position;
    pos.y = (obj->bounding_box.max.y + obj->bounding_box.min.y) * 0.5f;
    camera_screen_from_position(hud->camera, &pos, &screen_pos);

    int box_width = measure_text(FONT_DIALOG, interaction_name);

    screen_pos.x -= box_width >> 1;

    if (screen_pos.x < -SCREEN_EDGE_MARGIN || screen_pos.y < -SCREEN_EDGE_MARGIN ||
        screen_pos.x > SCREEN_WD + SCREEN_EDGE_MARGIN || screen_pos.y > SCREEN_HT + SCREEN_EDGE_MARGIN) {
        return;
    }

    screen_pos.x = floorf(screen_pos.x);
    screen_pos.y = floorf(screen_pos.y);

    rdpq_sync_pipe();
    material_pair_apply(hud->assets.overlay_material, NULL);
    rdpq_set_prim_color((color_t){0, 0, 0, 128});
    rdpq_texture_rectangle(
        TILE0, 
        screen_pos.x - TEXT_PADDING, screen_pos.y - BOX_HEIGHT - TEXT_PADDING, 
        screen_pos.x + box_width + TEXT_PADDING, screen_pos.y + TEXT_PADDING,
        0, 0
    );
    rdpq_sync_pipe();

    rdpq_text_printn(&(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_LEFT,
            .valign = VALIGN_BOTTOM,
            .width = 260,
            .height = 0,
            .wrap = WRAP_NONE,
        }, 
        FONT_DIALOG, 
        screen_pos.x, screen_pos.y, 
        interaction_name,
        strlen(interaction_name)
    );
}

void hud_draw_bar(int max_width, int current_width, int prev_width, int y, color_t color) {
    material_pair_apply(solid_primitive_material, NULL);
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

void hud_render_boss_name(hud_t* hud) {
    health_t* health = health_get(hud->boss.id);

    if (!health) {
        if (hud->boss.id) {
            hud_hide_boss_health(hud);
        }

        return;
    }

    int current_health = (int)(BOSS_HEALTH_WIDTH * health->current_health / health->max_health);

    hud_draw_bar(BOSS_HEALTH_WIDTH, current_health, 0, 20, health_color);
    rdpq_text_printn(&(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_LEFT,
            .valign = VALIGN_TOP,
            .width = BOSS_HEALTH_WIDTH,
            .height = BUTTON_ICON_SIZE,
            .wrap = WRAP_WORD,
        }, 
        FONT_DIALOG, 
        (320 - BOSS_HEALTH_WIDTH) >> 1, BOSS_HEALTH_Y + 8, 
        "Jelly King",
        strlen("Jelly King")
    );
}

void hud_render(void *data) {
    if (!update_has_layer(UPDATE_LAYER_WORLD)) {
        return;
    }
 
    struct hud* hud = (struct hud*)data;

    hud_render_boss_name(hud);

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

    rdpq_sync_pipe();
    
    hud_render_interaction_preview(hud);
}

void hud_init(struct hud* hud, struct player* player, camera_t* camera) {
    menu_add_callback(hud_render, hud, MENU_PRIORITY_HUD);
    hud->player = player;
    hud->camera = camera;
    live_cast_renderer_init(&hud->live_cast_renderer, &player->live_cast);
    font_type_use(FONT_DIALOG);
    hud->boss = (struct hud_boss){};
    
    hud->assets.overlay_material = material_cache_load("rom:/materials/menu/solid_primitive.mat");
}

void hud_destroy(struct hud* hud) {
    menu_remove_callback(hud);
    live_cast_renderer_destroy(&hud->live_cast_renderer);
    font_type_release(FONT_DIALOG);
    
    material_cache_release(hud->assets.overlay_material);
}

void hud_show_boss_health(struct hud* hud, const char* name, entity_id id) {
    strncpy(hud->boss.name, name, MAX_BOSS_NAME_LENGTH-1);
    hud->boss.name[MAX_BOSS_NAME_LENGTH-1] = '\0';
    hud->boss.id = id;
}

void hud_hide_boss_health(struct hud* hud) {
    hud->boss = (struct hud_boss){};
}