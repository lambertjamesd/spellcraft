#ifndef __MENU_SPELL_BUILDING_H__
#define __MENU_SPELL_BUILDING_H__

#include <stdint.h>

#include "../spell/spell.h"
#include "../player/inventory.h"

struct spell_building_menu {
    struct spell spell_copy;
    struct spell* current_spell;
};

void spell_building_menu_init(struct spell_building_menu* menu);
void spell_building_menu_destroy(struct spell_building_menu* menu);

void spell_building_menu_show(struct spell_building_menu* menu, struct spell* spell);
void spell_building_menu_hide(struct spell_building_menu* menu);

void spell_building_menu_update(struct spell_building_menu* menu);
void spell_building_render_menu(struct spell_building_menu* menu);


#endif
