#ifndef __MENU_HUD_H__
#define __MENU_HUD_H__

#include "../player/inventory.h"
#include "../render/material.h"
#include "../player/player.h"
#include "../render/material.h"
#include "live_cast_renderer.h"

struct hud {
    struct player* player;
    live_cast_renderer_t live_cast_renderer;
    material_t* button_icon;
    rdpq_font_t* action_font;
};

void hud_init(struct hud* hud, struct player* player);
void hud_destroy(struct hud* hud);

#endif