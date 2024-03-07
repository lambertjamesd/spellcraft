#ifndef __MENU_SPELL_BUILDING_H__
#define __MENU_SPELL_BUILDING_H__

#include <stdint.h>

#include "../spell/spell.h"

struct spell_building_menu {
    uint8_t symbol_cursor_x;
    uint8_t symbol_cursor_y;

    uint8_t spell_cursor_x;
    uint8_t spell_cursor_y;
};

void spell_building_menu_init(struct spell_building_menu* menu);
void spell_building_menu_destroy(struct spell_building_menu* menu);

void spell_building_menu_show(struct spell_building_menu* menu, struct spell* spell);
void spell_buliding_menu_hide(struct spell_building_menu* menu);

#endif
