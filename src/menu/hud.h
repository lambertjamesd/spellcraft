#ifndef __MENU_HUD_H__
#define __MENU_HUD_H__

#include "../player/inventory.h"
#include "../render/material.h"
#include "../player/player.h"
#include "../render/material.h"
#include "live_cast_renderer.h"

#define MAX_BOSS_NAME_LENGTH 16

struct hud_boss {
    char name[MAX_BOSS_NAME_LENGTH];
    entity_id id;
};

struct hud {
    struct player* player;
    live_cast_renderer_t live_cast_renderer;
    material_t* button_icon;
    struct hud_boss boss;
};

typedef struct hud hud_t;

void hud_init(struct hud* hud, struct player* player);
void hud_destroy(struct hud* hud);

void hud_show_boss_health(struct hud* hud, const char* name, entity_id id);
void hud_hide_boss_health(struct hud* hud);

#endif