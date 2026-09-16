#ifndef __MENU_SPELL_MENU_H__
#define __MENU_SPELL_MENU_H__

#include <stdint.h>
#include "../player/inventory.h"
#include "../render/material.h"
#include "../scene/scene_definition.h"
#include "../menu/rsp_menu.h"

#define SPELL_ICON_COUNT     32

struct spell_menu {    
    material_pair_t* solid_color;
    material_pair_t* spell_material;

    sprite_t* spell_icons[SPELL_ICON_COUNT];
    menu2d_vtx_t* spell_icon_vertices;

    float appear_timer;
    int appear_index;

    vector2_t offset;
    float scale;
};

typedef struct spell_menu spell_menu_t;

void spell_menu_init(struct spell_menu* spell_menu);
void spell_menu_destroy(struct spell_menu* spell_menu);

void spell_menu_show(struct spell_menu* spell_menu);
void spell_menu_hide(struct spell_menu* spell_menu);

void spell_menu_show_rune_upgrade(struct spell_menu* spell_menu, enum inventory_item_type item_type);

void spell_menu_update(struct spell_menu* spell_menu);
void spell_menu_render(struct spell_menu* spell_menu);

#endif