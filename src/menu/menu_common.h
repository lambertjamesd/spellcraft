#ifndef __MENU_MENU_COMMON_H__
#define __MENU_MENU_COMMON_H__

#include "../render/material.h"
#include "../spell/spell.h"

extern material_pair_t* menu_icons_material;
extern material_pair_t* menu_spell_icons[SPELL_ICON_COUNT];
extern material_pair_t* spell_cursor_material;
extern material_pair_t* solid_primitive_material;
extern material_pair_t* current_spell_icon;
extern material_pair_t* sprite_blit;

void menu_common_init();

void menu_common_render_background(int x, int y, int w, int h);

#endif