#include "menu_common.h"

#include "../resource/material_cache.h"

#include "../spell/spell.h"

static struct material* menu_background_material;
static struct material* menu_border_material;

struct material* menu_icons_material;
struct material* menu_spell_icons[SPELL_ICON_COUNT];
struct material* spell_cursor_material;
struct material* solid_primitive_material;
struct material* current_spell_icon;
struct material* sprite_blit;

static char* menu_spell_icon_filename[SPELL_ICON_COUNT] = {
    "rom:/materials/spell/icons/00_fire.mat",
    "rom:/materials/spell/icons/01_dash.mat",

    "rom:/materials/spell/icons/00_custom.mat",
    "rom:/materials/spell/icons/01_custom.mat",
    "rom:/materials/spell/icons/02_custom.mat",
    "rom:/materials/spell/icons/03_custom.mat",
    "rom:/materials/spell/icons/04_custom.mat",
    "rom:/materials/spell/icons/05_custom.mat",

};

void menu_common_init() {
    // material_cache_release() never called
    menu_background_material = material_cache_load("rom:/materials/menu/menu_corner.mat");
    // material_cache_release() never called
    menu_border_material = material_cache_load("rom:/materials/menu/menu_border.mat");
    // material_cache_release() never called
    menu_icons_material = material_cache_load("rom:/materials/menu/menu_icons.mat");
    // material_cache_release() never called
    spell_cursor_material = material_cache_load("rom:/materials/menu/menu_cursor.mat");
    // material_cache_release() never called
    solid_primitive_material = material_cache_load("rom:/materials/menu/solid_primitive.mat");
    // material_cache_release() never called
    current_spell_icon = material_cache_load("rom:/materials/menu/current_spell_icon.mat");
    // material_cache_release() never called
    sprite_blit = material_cache_load("rom:/materials/menu/sprite_blit.mat");

    for (int i = 0; i < SPELL_ICON_COUNT; i += 1) {
        // material_cache_release() never called
        menu_spell_icons[i] = material_cache_load(menu_spell_icon_filename[i]);
    }
}

void menu_common_render_background(int x, int y, int w, int h) {
    rspq_block_run(menu_border_material->block);

    rdpq_texture_rectangle(
        TILE0,
        x - 3, y - 3,
        x, y, 
        0, 0
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x, y - 3,
        x + w, y, 
        3, 0,
        1, 3
    );

    rdpq_texture_rectangle(
        TILE0,
        x + w, y - 3,
        x + w + 3, y, 
        6, 0
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x - 3, y,
        x, y + h, 
        0, 3,
        3, 1
    );

    rdpq_texture_rectangle(
        TILE0,
        x - 3, y + h,
        x, y + h + 3, 
        0, 6
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x, y + h,
        x + w, y + h + 3, 
        3, 6,
        1, 3
    );

    rdpq_texture_rectangle(
        TILE0,
        x + w, y + h,
        x + w + 3, y + h + 3, 
        0, 6
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x + w, y,
        x + w + 3, y + h, 
        6, 3,
        3, 1
    );

    rspq_block_run(menu_background_material->block);

    rdpq_texture_rectangle_scaled(
        TILE0,
        x, y,
        x + w, y + h,
        0, 0,
        64, 64
    );
}