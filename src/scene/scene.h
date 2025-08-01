#ifndef __SCENE_SCENE_H__
#define __SCENE_SCENE_H__

#include "../render/render_batch.h"
#include "../render/camera.h"
#include "../render/tmesh.h"
#include "../collision/mesh_collider.h"

#include "../player/player.h"
#include "../player/inventory.h"
#include "../menu/pause_menu.h"
#include "../menu/hud.h"
#include "../overworld/overworld.h"
#include "camera_controller.h"
#include "camera_animation.h"

typedef void(*entity_init)(void* entity, void* definition);
typedef void(*entity_destroy)(void* entity);

struct entity_field_type_location {
    uint8_t type;
    uint8_t offset;
};

enum entity_field_types {
    ENTITY_FIELD_TYPE_STRING,
};

struct entity_definition {
    const char* name;
    entity_init init;
    entity_destroy destroy;
    uint16_t entity_size;
    uint16_t definition_size;
    struct entity_field_type_location* fields;
    uint16_t field_count;
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
    char* scene_name;
};

struct named_location {
    char* name;
    struct Vector3 position;
    struct Vector2 rotation;
    uint16_t room_index;
};

struct static_entity_range {
    uint16_t start;
    uint16_t end;
};

struct scene {
    struct static_entity* static_entities;
    struct static_entity_range* room_static_ranges;

    struct mesh_collider mesh_collider;

    struct player player;
    struct Camera camera;
    struct camera_controller camera_controller;

    struct pause_menu pause_menu;
    struct hud hud;
    
    struct entity_data* entity_data;
    struct loading_zone* loading_zones;
    struct named_location* named_locations;
    struct overworld* overworld;

    uint16_t room_count;
    uint16_t static_entity_count;
    uint16_t entity_data_count;
    uint16_t loading_zone_count;
    uint16_t named_location_count;

    uint16_t current_room;
    uint16_t preview_room;

    char* string_table;
    char* scene_vars;

    struct camera_animation_list camera_animations;
};

extern struct scene* current_scene;

void scene_render(void* data, struct render_batch* batch);
void scene_update(void* data);

void scene_queue_next(char* scene_name);
void scene_clear_next();

bool scene_has_next();

char* scene_get_next();
char* scene_get_next_entry();

struct named_location* scene_find_location(char* name);

void scene_entity_apply_types(void* definition, char* string_table, struct entity_field_type_location* type_locations, int type_location_count);

#endif