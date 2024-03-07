#ifndef __SCENE_WORLD_H__
#define __SCENE_WORLD_H__

#include "../render/mesh.h"
#include "../render/render_batch.h"
#include "../render/camera.h"
#include "../collision/mesh_collider.h"

#include "player.h"
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

enum STATIC_ENTITY_FLAGS {
    STATIC_ENTITY_FLAGS_EMBEDDED_MESH = (1 << 0),
};

struct static_entity {
    struct mesh* mesh;
    uint8_t flags;
};

struct world {
    struct static_entity* static_entities;
    uint16_t static_entity_count;

    struct mesh_collider mesh_collider;

    struct player player;
    struct Camera camera;
    struct camera_controller camera_controller;
    
    struct entity_data* entity_data;
    uint16_t entity_data_count;
};

void world_render(void* data, struct render_batch* batch);

#endif