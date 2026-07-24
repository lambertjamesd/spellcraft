#include "pinwheel.h"    
    
#include "../collision/shapes/cylinder.h"

#define ROTATION_SCALAR     2.0f

static dynamic_object_type_t fan_collider = {
    CYLINDER_COLLIDER(0.25f, 0.6f),
    .center = { 0.0f, 0.6f, 0.0f }, 
};

void pinwheel_update(void* data) {
    pinwheel_t* pinwheel = (pinwheel_t*)data;

    struct Vector3* vel = &pinwheel->collider.velocity;
    
    float angular_velocity = -vector3Dot(vel, &pinwheel->forward) * ROTATION_SCALAR;
    vector3Scale(&pinwheel->collider.velocity, &pinwheel->collider.velocity, 0.9f);
    
    float abs_vel = fabsf(angular_velocity);

    if (abs_vel > 0.01f) {
        quaternion_t rotation_delta;
        quatAxisAngle(&gUp, angular_velocity * fixed_time_step, &rotation_delta);
    
        quaternion_t final_rotation;
        quatMultiply(&pinwheel->renderable.mesh_render.armature.pose[1].rotation, &rotation_delta, &final_rotation);
        pinwheel->renderable.mesh_render.armature.pose[1].rotation = final_rotation;
    }
}

void pinwheel_init(pinwheel_t* pinwheel, struct pinwheel_definition* definition, entity_id entity_id) {
    transformInit(&pinwheel->transform, &definition->position, &definition->rotation, &gOneVec);
    renderable_init(&pinwheel->renderable, &pinwheel->transform, "rom:/meshes/objects/env_interactive/pinwheel.tmesh");

    render_scene_add_renderable(&pinwheel->renderable, 1.0f);

    dynamic_object_init(
        entity_id,
        &pinwheel->collider,
        &fan_collider,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY,
        &pinwheel->transform.position,
        NULL
    );

    pinwheel->collider.is_fixed = 1;
    pinwheel->collider.weight_class = WEIGHT_CLASS_HEAVY;
    
    collision_scene_add(&pinwheel->collider);
    
    update_add(pinwheel, pinwheel_update, UPDATE_PRIORITY_PHYICS, UPDATE_LAYER_WORLD);

    quatMultVector(&pinwheel->transform.rotation, &gRight, &pinwheel->forward);
}

void pinwheel_destroy(pinwheel_t* pinwheel, struct pinwheel_definition* definition) {
    render_scene_remove(&pinwheel->renderable);
    renderable_destroy(&pinwheel->renderable);
    collision_scene_remove(&pinwheel->collider);
    update_remove(pinwheel);
}

void pinwheel_common_init() {

}

void pinwheel_common_destroy() {

}
