#include "spell_building_menu.h"

#include <libdragon.h>

#include "menu_rendering.h"
#include "menu_common.h"
#include "../resource/material_cache.h"
#include "../time/time.h"

static struct material* spell_symbol_material;

#define SPELL_SYMBOLS_PER_ROW   6

#define SPELL_SYMBOL_LOCATION_X(index)  ((((index) - SPELL_SYMBOL_FIRE) % SPELL_SYMBOLS_PER_ROW) * 32 + 40)
#define SPELL_SYMBOL_LOCATION_Y(index)  ((((index) - SPELL_SYMBOL_FIRE) / SPELL_SYMBOLS_PER_ROW) * 32 + 142)

void spell_building_render_menu(struct spell_building_menu* menu) {
    menu_common_render_background(32, 32, 256, 96);

    menu_common_render_background(32, 134, 256, 74);
}

void spell_building_menu_update(struct spell_building_menu* menu) {

}

void spell_building_menu_init(struct spell_building_menu* menu) {
    spell_symbol_material = material_cache_load("rom:/materials/spell/symbols.mat");
}

void spell_building_menu_destroy(struct spell_building_menu* menu) {
    material_cache_release(spell_symbol_material);
}

void spell_building_menu_show(struct spell_building_menu* menu, struct spell* spell) {
    menu->spell_copy = *spell;
    menu->current_spell = spell;
}

void spell_building_menu_hide(struct spell_building_menu* menu) {
    *menu->current_spell = menu->spell_copy;
}