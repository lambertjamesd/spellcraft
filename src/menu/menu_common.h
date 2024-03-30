#ifndef __MENU_MENU_COMMON_H__
#define __MENU_MENU_COMMON_H__

#include "../render/material.h"
#include "../spell/spell.h"

extern struct material* menu_icons_material;
extern struct material* menu_spell_icons[SPELL_ICON_COUNT];
extern struct material* spell_cursor_material;

void menu_common_init();

void menu_common_render_background(int x, int y, int w, int h);

#endif