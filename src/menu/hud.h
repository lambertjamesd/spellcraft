#ifndef __MENU_HUD_H__
#define __MENU_HUD_H__

#include "../player/inventory.h"
#include "../render/material.h"

struct hud {
    int padding;
};

void hud_init(struct hud* hud);
void hud_destroy(struct hud* hud);

#endif