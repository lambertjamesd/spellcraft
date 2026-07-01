#ifndef __MENU_MAP_H__
#define __MENU_MAP_H__

#include "../render/tmesh.h"
#include "../scene/scene_definition.h"
#include "../math/vector2s16.h"
#include "../math/vector2.h"
#include "../resource/material_cache.h"
#include <stdint.h>

struct menu_map_icon {
    vector2s16_t pos;
    boolean_variable hidden;
    uint8_t icon_index;
};

typedef struct menu_map_icon menu_map_icon_t;

struct menu_map_room {
    tmesh_t outline;
    menu_map_icon_t *icons;
    uint16_t icon_count;
};

typedef struct menu_map_room menu_map_room_t;

struct menu_map {
    menu_map_room_t* rooms;
    uint16_t room_count;
    vector2_t min, max;
    
    material_pair_t* outline_material;
};

typedef struct menu_map menu_map_t;

void menu_map_load(menu_map_t* map, FILE* file);
void menu_map_destroy(menu_map_t* map);

void menu_map_render(menu_map_t* map, vector2s16_t* min, vector2s16_t* max);

#endif