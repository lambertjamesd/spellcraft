#include "menu_common.h"

#include "../resource/material_cache.h"

static struct material* menu_background_material;
static struct material* menu_border_material;

void menu_common_init() {
    menu_background_material = material_cache_load("rom:/materials/menu/menu_corner.mat");
    menu_border_material = material_cache_load("rom:/materials/menu/menu_border.mat");
}

void menu_common_render_background(int x, int y, int w, int h) {
    rspq_block_run(menu_background_material->block);

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