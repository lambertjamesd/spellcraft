#ifndef __MENU_PAUSE_MENU_H__
#define __MENU_PAUSE_MENU_H__

#include "../player/inventory.h"
#include "spell_building_menu.h"
#include "spell_menu.h"

enum active_menu {
    ACTIVE_MENU_NONE,
    ACTIVE_MENU_SPELLS,
    ACTIVE_MENU_SPELL_BUILDING,
};

struct pause_menu {
    struct spell_building_menu spell_building_menu;
    struct spell_menu spell_menu;
    enum active_menu active_menu;
};

void pause_menu_init(struct pause_menu* pause_menu);
void pause_menu_destroy(struct pause_menu* pause_menu);

#endif