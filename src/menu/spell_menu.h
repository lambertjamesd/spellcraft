#ifndef __MENU_SPELL_MENU_H__
#define __MENU_SPELL_MENU_H__

#include <stdint.h>
#include "../player/inventory.h"

struct spell_menu {
    uint16_t cursor_x;
    uint16_t cursor_y;
};

void spell_menu_init(struct spell_menu* spell_menu);
void spell_menu_destroy(struct spell_menu* spell_menu);

void spell_menu_show(struct spell_menu* spell_menu);
void spell_menu_hide(struct spell_menu* spell_menu);

struct spell* spell_menu_update(struct spell_menu* spell_menu);
void spell_menu_render(struct spell_menu* spell_menu);

#endif