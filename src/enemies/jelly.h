#ifndef __ENEMIES_JELLY_H__
#define __ENEMIES_JELLY_H__

#include <stdbool.h>
#include "../collision/dynamic_object.h"
#include "../collision/spatial_trigger.h"
#include "../entity/health.h"
#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../render/tmesh.h"
#include "../scene/scene_definition.h"
#include "../effects/drop_shadow.h"
#include "../entity/damage.h"

struct jelly {
    struct TransformSingleAxis transform;
    struct tmesh* mesh;
    struct tmesh* ice_mesh;
    struct health health;
    struct dynamic_object collider;
    struct spatial_trigger vision;

    struct Vector3 shear_spring;
    struct Vector3 shear_velocity;

    float jump_timer;

    float freeze_timer;

    entity_id current_target;

    uint16_t needs_new_radius: 1;
    uint16_t is_frozen: 1;
    uint16_t is_jumping: 1; 
    uint16_t is_attacking: 1;
    uint16_t is_active: 1;

    struct drop_shadow drop_shadow;
};

void jelly_init(struct jelly* jelly, struct jelly_definition* definition, entity_id id);
void jelly_destroy(struct jelly* jelly);

bool jelly_get_is_active(struct jelly* jelly);

void jelly_launch_attack(struct jelly* jelly, struct Vector3* velocity, int collision_group, entity_id target);

void jelly_reset_collision_group(struct jelly* jelly);

#endif