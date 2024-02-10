#ifndef __SCENE_WORLD_H__
#define __SCENE_WORLD_H__

#include "../render/mesh.h"

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
};

void world_render(struct world* world);

#endif