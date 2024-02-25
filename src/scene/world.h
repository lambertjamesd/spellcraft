#ifndef __SCENE_WORLD_H__
#define __SCENE_WORLD_H__

#include "../render/mesh.h"
#include "../render/render_batch.h"
#include "../render/camera.h"
#include "../collision/mesh_collider.h"

#include "player.h"
#include "camera_controller.h"

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

    int static_render_id;

    struct player player;
    struct Camera camera;
    struct camera_controller camera_controller;
};

void world_render(void* data, struct render_batch* batch);

#endif