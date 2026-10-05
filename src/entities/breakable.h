#ifndef __ENTITIES_BREAKABLE_H__
#define __ENTITIES_BREAKABLE_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "entity_deps.h"
#include "../effects/mesh_animation.h"

typedef struct {
    transform_t* transform;
    vector3_t start_scale;
    vector3_t velocity;
    vector3_t angular_velocity;
} breakable_shard_t;

struct breakable {
    transform_sa_t transform;
    renderable_t renderable;
    animator_t animator;
    dynamic_object_t collider;
    health_t health;
    interactable_t interactable;
    tmesh_t* mesh;
    tmesh_t* break_effect_mesh;
    animation_set_t* break_animations;
    breakable_shard_t* shards;
    uint8_t shard_count;
    bool is_breaking;
    float break_timer;
};

typedef struct breakable breakable_t;

void breakable_init(breakable_t* breakable, struct breakable_definition* definition, entity_id entity_id);
void breakable_destroy(breakable_t* breakable, struct breakable_definition* definition);
void breakable_common_init();
void breakable_common_destroy();

#endif