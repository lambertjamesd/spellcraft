#ifndef __SCENE_WORLD_H__
#define __SCENE_WORLD_H__

#include "../render/render_batch.h"
#include "../render/camera.h"
#include "../render/tmesh.h"
#include "../collision/mesh_collider.h"

#include "../player/player.h"
#include "../player/inventory.h"
#include "../menu/pause_menu.h"
#include "../menu/hud.h"
#include "camera_controller.h"

typedef void(*entity_init)(void* entity, void* definition);
typedef void(*entity_destroy)(void* entity);

struct entity_definition {
    const char* name;
    entity_init init;
    entity_destroy destroy;
    uint16_t entity_size;
    uint16_t definition_size;
};

struct entity_data {
    struct entity_definition* definition;
    void* entities;
    uint16_t entity_count;
};

struct static_entity {
    struct tmesh tmesh;
};

struct loading_zone {
    struct Box3D bounding_box;
    char* world_name;
};

struct world {
    struct static_entity* static_entities;

    struct mesh_collider mesh_collider;

    struct player player;
    struct Camera camera;
    struct camera_controller camera_controller;

    struct pause_menu pause_menu;
    struct hud hud;
    
    struct entity_data* entity_data;
    struct loading_zone* loading_zones;

    uint16_t static_entity_count;
    uint16_t entity_data_count;
    uint16_t loading_zone_count;

    char* string_table;
};

void world_render(void* data, struct render_batch* batch);
void world_update(void* data);

void world_queue_next(char* world_name);
void world_clear_next();

bool world_has_next();

char* world_get_next();
char* world_get_next_entry();

#endif