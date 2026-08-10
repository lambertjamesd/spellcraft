#ifndef __ENTITIES_COMM_STONE_H__
#define __ENTITIES_COMM_STONE_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "../entities/entity_deps.h"

struct comm_stone {
    transform_t transform;
    renderable_t renderable;
    vector3_t target;
    vector3_t from;

    float activation_lerp;
    bool is_active;
    bool is_visible;
};

typedef struct comm_stone comm_stone_t;

void comm_stone_init(comm_stone_t* comm_stone, struct comm_stone_definition* definition, entity_id entity_id);
void comm_stone_destroy(comm_stone_t* comm_stone, struct comm_stone_definition* definition);

void comm_stone_activate(comm_stone_t* stone, vector3_t* from, vector3_t* target);
void comm_stone_deactivate(comm_stone_t* stone);

bool comm_stone_is_animating(comm_stone_t* stone);

void comm_stone_common_init();
void comm_stone_common_destroy();

#endif