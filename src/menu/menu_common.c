#include "menu_common.h"

#include "../resource/material_cache.h"

#include "../spell/spell.h"

static material_pair_t* menu_background_material;
static material_pair_t* menu_border_material;
static material_pair_t* menu_border_TR_material;

material_pair_t* menu_icons_material;
material_pair_t* menu_spell_icons[SPELL_ICON_COUNT];
material_pair_t* spell_cursor_material;
material_pair_t* solid_primitive_material;
material_pair_t* current_spell_icon;
material_pair_t* sprite_blit;

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
    menu_border_TR_material = material_cache_load("rom:/materials/menu/menu_border_transparent.mat");
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
    material_pair_apply(menu_border_material, NULL);

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
        6, 3
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
        3, 6
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
        6, 9
    );

    rdpq_texture_rectangle(
        TILE0,
        x + w, y + h,
        x + w + 3, y + h + 3, 
        6, 6
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x + w, y,
        x + w + 3, y + h, 
        6, 3,
        9, 6
    );

    material_pair_apply(menu_background_material, NULL);

    rdpq_texture_rectangle_scaled(
        TILE0,
        x, y,
        x + w, y + h,
        0, 0,
        64, 64
    );
}

#define TR_IM_SIZE              16
#define TR_PADDING             5
#define TR_MARGIN              3

#define TR_CORNER_SIZE         7

void menu_common_render_background_transparent(int x, int y, int w, int h) {
    material_pair_apply(menu_border_TR_material, NULL);

    // TL
    rdpq_texture_rectangle(
        TILE0,
        x - TR_PADDING, y - TR_PADDING,
        x + TR_MARGIN, y + TR_MARGIN,
        0, 0
    );

    // T
    rdpq_texture_rectangle_scaled(
        TILE0,
        x+TR_MARGIN, y - TR_PADDING,
        x + w-TR_MARGIN, y + TR_MARGIN, 
        TR_CORNER_SIZE, 0,
        TR_IM_SIZE-TR_CORNER_SIZE, TR_PADDING + TR_MARGIN
    );

    // TR
    rdpq_texture_rectangle(
        TILE0,
        x + w-TR_MARGIN, y - TR_PADDING,
        x + w+TR_PADDING, y + TR_MARGIN, 
        TR_IM_SIZE-TR_PADDING-TR_MARGIN, 0
    );

    // L
    rdpq_texture_rectangle_scaled(
        TILE0,
        x-TR_PADDING, y+TR_MARGIN,
        x+TR_MARGIN, y + h-TR_MARGIN, 
        0, TR_CORNER_SIZE,
        TR_PADDING+TR_MARGIN, TR_IM_SIZE-TR_CORNER_SIZE
    );

    // BL
    rdpq_texture_rectangle(
        TILE0,
        x - TR_PADDING, y + h - TR_MARGIN,
        x + TR_MARGIN, y + h + TR_PADDING, 
        0, TR_IM_SIZE - TR_PADDING - TR_MARGIN
    );

    // B
    rdpq_texture_rectangle_scaled(
        TILE0,
        x+TR_MARGIN, y + h-TR_MARGIN,
        x + w-TR_MARGIN, y + h + TR_PADDING, 
        TR_CORNER_SIZE, TR_IM_SIZE - TR_PADDING - TR_MARGIN,
        TR_IM_SIZE-TR_CORNER_SIZE, TR_IM_SIZE
    );

    // BR
    rdpq_texture_rectangle(
        TILE0,
        x + w-TR_MARGIN, y + h-TR_MARGIN,
        x + w + TR_PADDING, y + h + TR_PADDING, 
        TR_IM_SIZE-TR_MARGIN-TR_PADDING, TR_IM_SIZE-TR_MARGIN-TR_PADDING
    );

    // R
    rdpq_texture_rectangle_scaled(
        TILE0,
        x + w-TR_MARGIN, y+TR_MARGIN,
        x + w + TR_PADDING, y + h-TR_MARGIN, 
        TR_IM_SIZE-TR_MARGIN-TR_PADDING, TR_CORNER_SIZE,
        TR_IM_SIZE, TR_IM_SIZE-TR_CORNER_SIZE
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x+TR_MARGIN, y+TR_MARGIN,
        x + w-TR_MARGIN, y + h-TR_MARGIN,
        TR_CORNER_SIZE, TR_CORNER_SIZE,
        TR_IM_SIZE-TR_CORNER_SIZE, TR_IM_SIZE-TR_CORNER_SIZE
    );
}