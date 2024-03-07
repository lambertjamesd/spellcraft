#include "spell_building_menu.h"

#include "menu_rendering.h"
#include "../resource/material_cache.h"

struct material* spell_symbol_material;

void spell_building_menu_menu(struct spell_building_menu* menu) {
    glCallList(spell_symbol_material->list);

    rdpq_texture_rectangle_scaled(
        TILE0,
        32, 32,
        56, 56,
        0, 0,
        24, 24
    );
}

void spell_building_menu_init(struct spell_building_menu* menu) {
    spell_symbol_material = material_cache_load("rom:/materials/spell/symbols.mat");
}

void spell_building_menu_destroy(struct spell_building_menu* menu) {
    
}

void spell_building_menu_show(struct spell_building_menu* menu, struct spell* spell) {
    menu_add_callback((menu_render_callback)spell_building_menu_menu, menu, 0);
}

void spell_buliding_menu_hide(struct spell_building_menu* menu) {
    menu_remove_callback(menu);
}