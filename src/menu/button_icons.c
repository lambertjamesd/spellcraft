#include "button_icons.h"

#include <libdragon.h>
#include "../resource/material_cache.h"
#include "../resource/sprite_cache.h"

static material_pair_t* button_material;
static sprite_t* button_icon_sprites[8];
static uint8_t button_icon_ref_count[8];
static uint8_t button_material_ref_count;
static const char sprite_names[] = {
    'a',
    'b',
    'z',
    's',
    'd',
    'l',
    'r',
    'c',
};

struct button_icon_data {
    uint8_t icon_index;
    uint8_t rotation;
};

typedef struct button_icon_data button_icon_data_t;

static button_icon_data_t icon_data[] = {
    [BUTTON_TYPE_A] = {.icon_index = 0, .rotation = 0},
    [BUTTON_TYPE_B] = {.icon_index = 1, .rotation = 0},
    [BUTTON_TYPE_Z] = {.icon_index = 2, .rotation = 0},
    [BUTTON_TYPE_START] = {.icon_index = 3, .rotation = 0},
    [BUTTON_TYPE_D_U] = {.icon_index = 4, .rotation = 0},
    [BUTTON_TYPE_D_D] = {.icon_index = 4, .rotation = 1},
    [BUTTON_TYPE_D_L] = {.icon_index = 4, .rotation = 2},
    [BUTTON_TYPE_D_R] = {.icon_index = 4, .rotation = 3},
    [BUTTON_TYPE_Y] = {.icon_index = 0, .rotation = 0},
    [BUTTON_TYPE_X] = {.icon_index = 0, .rotation = 0},
    [BUTTON_TYPE_L] = {.icon_index = 5, .rotation = 0},
    [BUTTON_TYPE_R] = {.icon_index = 6, .rotation = 0},
    [BUTTON_TYPE_C_U] = {.icon_index = 7, .rotation = 0},
    [BUTTON_TYPE_C_D] = {.icon_index = 7, .rotation = 1},
    [BUTTON_TYPE_C_L] = {.icon_index = 7, .rotation = 2},
    [BUTTON_TYPE_C_R] = {.icon_index = 7, .rotation = 3},
};

void button_icon_load(enum button_type type) {
    if (button_material_ref_count == 0) {
        button_material = material_cache_load("rom:/materials/menu/button_icons.mat");
    }
    button_material_ref_count += 1;

    int button_index = icon_data[type].icon_index;

    if (button_icon_ref_count[button_index] == 0) {
        char filename[40];
        sprintf(filename, "rom:/images/menu/button_icons_%c.sprite", sprite_names[button_index]);
        button_icon_sprites[button_index] = sprite_cache_load(filename);
    }
    
    button_icon_ref_count[button_index] += 1;
} 

void button_icon_unload(enum button_type type) {
    assert(button_material_ref_count > 0);

    button_material_ref_count -= 1;

    if (!button_material_ref_count) {
        material_cache_release(button_material);
        button_material = NULL;
    }

    int button_index = icon_data[type].icon_index;

    assert(button_icon_ref_count[button_index] > 0);
    button_icon_ref_count[button_index] -= 1;

    if (!button_icon_ref_count[button_index]) {
        sprite_cache_release(button_icon_sprites[button_index]);
        button_icon_sprites[button_index] = NULL;
    }
}

void button_icon_draw(enum button_type type, int x, int y) {
    button_icon_data_t icon = icon_data[type];

    rdpq_sprite_upload(TILE0, button_icon_sprites[icon.icon_index], NULL);
    material_apply(&button_material->apply);

    rdpq_texture_rectangle_raw(TILE0, x, y, x + 16, y + 16, 0, 0, 1, 1);
}