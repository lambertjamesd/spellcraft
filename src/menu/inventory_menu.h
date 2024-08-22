#ifndef __MENU_INVENTORY_MENU__
#define __MENU_INVENTORY_MENU__

#include <stdint.h>
#include "../player/inventory.h"

struct inventory_menu {
    uint16_t cursor_x;
    uint16_t cursor_y;
};

void inventory_menu_init(struct inventory_menu* memu);
void inventory_menu_destroy(struct inventory_menu* memu);

void inventory_menu_show(struct inventory_menu* menu);
void inventory_menu_hide(struct inventory_menu* menu);

void inventory_menu_update(struct inventory_menu* menu);
void inventory_menu_render(struct inventory_menu* menu);

#endif