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
#include "../entity/entity_spawner.h"

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

struct room_entities {
    void* block;
    uint16_t block_size;
};

typedef struct room_entities room_entities_t;

#define ROOM_INDEX_NONE     0xFFFF

struct loaded_room {
    uint16_t room_index;
    uint16_t entity_count;
    entity_id* entity_ids;
};

typedef struct loaded_room loaded_room_t;

#define MAX_LOADED_ROOM 4

struct scene {
    struct static_entity* static_entities;
    struct static_entity_range* room_static_ranges;

    struct mesh_collider mesh_collider;

    struct player player;
    struct Camera camera;
    struct camera_controller camera_controller;

    struct pause_menu pause_menu;
    struct hud hud;
    
    struct loading_zone* loading_zones;
    struct named_location* named_locations;
    struct overworld* overworld;

    room_entities_t* room_entities;

    uint16_t room_count;
    uint16_t static_entity_count;
    uint16_t loading_zone_count;
    uint16_t named_location_count;

    loaded_room_t loaded_rooms[MAX_LOADED_ROOM];

    char* string_table;
    char* scene_vars;

    struct camera_animation_list camera_animations;
};

extern struct scene* current_scene;

void scene_render(void* data, struct render_batch* batch);
void scene_update(void* data);

void scene_queue_next(char* scene_name);
void scene_clear_next();

bool scene_show_room(struct scene* scene, int room_index);
void scene_hide_room(struct scene* scene, int room_index);

bool scene_is_showing_room(struct scene* scene, int room_index);

bool scene_has_next();

char* scene_get_next();
char* scene_get_next_entry();

struct named_location* scene_find_location(char* name);

void scene_entity_apply_types(void* definition, char* string_table, struct entity_field_type_location* type_locations, int type_location_count);

#endif