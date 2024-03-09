#ifndef __MENU_PAUSE_MENU_H__
#define __MENU_PAUSE_MENU_H__

#include "../player/inventory.h"
#include "spell_building_menu.h"

struct pause_menu {
    struct inventory* inventory;
    struct spell_building_menu spell_menu;
    uint16_t is_open: 1;
};

void pause_menu_init(struct pause_menu* pause_menu, struct inventory* inventory);
void pause_menu_destroy(struct pause_menu* pause_menu);

#endif