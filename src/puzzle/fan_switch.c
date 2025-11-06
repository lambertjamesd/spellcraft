#include "fan_switch.h"

#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "../collision/shapes/cylinder.h"
#include "../time/time.h"
#include "../cutscene/expression_evaluate.h"

#define ROTATION_SCALAR     2.0f
#define OUTPUT_THRESHOLD    4.0f

static dynamic_object_type_t fan_collider = {
    CYLINDER_COLLIDER(0.25f, 0.6f),
};

void fan_switch_update(void* data) {
    fan_switch_t* fan_switch = (fan_switch_t*)data;

    struct Vector3* vel = &fan_switch->collider.velocity;
    struct Vector2* angle = &fan_switch->transform.rotation;
    
    float angular_velocity = (vel->x * angle->y - vel->z * angle->x) * ROTATION_SCALAR;
    vector3Scale(&fan_switch->collider.velocity, &fan_switch->collider.velocity, 0.9f);
    
    float abs_vel = fabsf(angular_velocity);

    expression_set_bool(fan_switch->output, abs_vel > OUTPUT_THRESHOLD);

    if (abs_vel > 0.01f) {
        quaternion_t rotation_delta;
        quatAxisAngle(&gUp, angular_velocity * fixed_time_step, &rotation_delta);
    
        quaternion_t final_rotation;
        quatMultiply(&fan_switch->renderable.armature.pose[1].rotation, &rotation_delta, &final_rotation);
        fan_switch->renderable.armature.pose[1].rotation = final_rotation;
    }
}

void fan_switch_init(fan_switch_t* fan_switch, struct fan_switch_definition* definition, entity_id entity_id) {
    transformSaInit(&fan_switch->transform, &definition->position, &definition->rotation, 1.0f);
    renderable_single_axis_init(&fan_switch->renderable, &fan_switch->transform, "rom:/meshes/puzzle/fan_switch.tmesh");
    
    render_scene_add_renderable(&fan_switch->renderable, 1.5f);

    dynamic_object_init(
        entity_id,
        &fan_switch->collider,
        &fan_collider,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY,
        &fan_switch->transform.position,
        NULL
    );

    fan_switch->collider.center.y = fan_collider.data.cylinder.half_height;
    fan_switch->collider.is_fixed = 1;
    fan_switch->collider.weight_class = WEIGHT_CLASS_HEAVY;

    collision_scene_add(&fan_switch->collider);

    update_add(fan_switch, fan_switch_update, UPDATE_PRIORITY_PHYICS, UPDATE_LAYER_WORLD);

    fan_switch->output = definition->output;
}

void fan_switch_destroy(fan_switch_t* fan_switch) {
    render_scene_remove(&fan_switch->renderable);
    renderable_destroy(&fan_switch->renderable);
    collision_scene_remove(&fan_switch->collider);
    update_remove(fan_switch);
}