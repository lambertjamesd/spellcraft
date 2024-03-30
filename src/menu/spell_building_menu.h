#ifndef __MENU_SPELL_BUILDING_H__
#define __MENU_SPELL_BUILDING_H__

#include <stdint.h>

#include "../spell/spell.h"

struct spell_building_menu {
    uint8_t symbol_cursor_x;
    uint8_t symbol_cursor_y;

    uint8_t spell_cursor_x;
    uint8_t spell_cursor_y;

    struct spell_symbol symbol_grid[SPELL_MAX_ROWS][SPELL_MAX_COLS];

    struct spell* current_spell;
};

void spell_building_menu_init(struct spell_building_menu* menu);
void spell_building_menu_destroy(struct spell_building_menu* menu);

void spell_building_menu_show(struct spell_building_menu* menu, struct spell* spell);
void spell_building_menu_hide(struct spell_building_menu* menu);

void spell_building_menu_update(struct spell_building_menu* menu);
void spell_building_render_menu(struct spell_building_menu* menu);


#endif
