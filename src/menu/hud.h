#ifndef __MENU_HUD_H__
#define __MENU_HUD_H__

#include "../player/inventory.h"
#include "../render/material.h"

struct hud {
    struct inventory* inventory;
    struct material* current_spell_icon;
};

void hud_init(struct hud* hud, struct inventory* inventory);
void hud_destroy(struct hud* hud);

#endif