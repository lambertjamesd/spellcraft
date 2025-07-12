#include "jelly_king.h"

#include "../collision/shapes/cylinder.h"
#include "../render/render_scene.h"
#include "../resource/animation_cache.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"
#include "../math/constants.h"

#define VISION_DISTANCE 12.0f

#define MAX_HEALTH      1000.0f

#define MAX_ROTATE_PER_SECOND   20.0f
#define SPEED                   20.0f
#define ACCEL                   1.5f

static struct dynamic_object_type jelly_king_collider = {
    CYLINDER_COLLIDER(2.0f, 1.5f),
    .friction = 0.1f,
    .bounce = 0.0f,
    // about a 40 degree slope
    .max_stable_slope = 0.219131191f,
};

static struct spatial_trigger_type jelly_king_vision_type = {
    .type = SPATIAL_TRIGGER_CYLINDER,
    .data = {
        .cylinder = {
            .radius = VISION_DISTANCE,
            .half_height = VISION_DISTANCE,
        },
    },
};

void jelly_king_idle(struct jelly_king* jelly_king) {
    struct contact* nearest_target = dynamic_object_nearest_contact(jelly_king->vision.active_contacts, &jelly_king->transform.position);

    if (!nearest_target) {
        return;
    }

    jelly_king->state = JELLY_KING_MOVE_TO_TARGET;
}

void jelly_king_move_to_target(struct jelly_king* jelly_king) {
    struct contact* nearest_target = dynamic_object_nearest_contact(jelly_king->vision.active_contacts, &jelly_king->transform.position);

    if (!nearest_target) {
        return;
    }

    struct Vector3 offset;
    vector3Sub(&nearest_target->point, &jelly_king->transform.position, &offset);

    struct Vector2 targetRotation;
    vector2LookDir(&targetRotation, &offset);

    vector2RotateTowards(&jelly_king->transform.rotation, &targetRotation, &jelly_king->max_rotate, &jelly_king->transform.rotation);

    if (vector2Dot(&targetRotation, &jelly_king->transform.rotation) > 0.5f) {
        struct Vector3 targetVel;
        vector2ToLookDir(&jelly_king->transform.rotation, &targetVel);
        vector3Scale(&targetVel, &targetVel, SPEED);

        vector3MoveTowards(&jelly_king->collider.velocity, &targetVel, ACCEL * fixed_time_step, &jelly_king->collider.velocity);
    }

    if (dynamic_object_find_contact(&jelly_king->collider, nearest_target->other_object)) {
        animator_run_clip(&jelly_king->animator, jelly_king->animations.attack, 0.0f, false);
        jelly_king->state = JELLY_KING_ATTACK;
    }
}

void jelly_king_attack(struct jelly_king* jelly_king) {
    if (!animator_is_running(&jelly_king->animator)) {
        jelly_king->state = JELLY_KING_MOVE_TO_TARGET;
    }
}

void jelly_king_update(void* data) {
    struct jelly_king* jelly_king = (struct jelly_king*)data;
    animator_update(&jelly_king->animator, jelly_king->renderable.armature.pose, fixed_time_step);

    switch (jelly_king->state)
    {
        case JELLY_KING_IDLE:
            jelly_king_idle(jelly_king);
            break;
        case JELLY_KING_MOVE_TO_TARGET:
            jelly_king_move_to_target(jelly_king);
            break;
        case JELLY_KING_ATTACK:
            jelly_king_attack(jelly_king);
            break;
    }
}

void jelly_king_init(struct jelly_king* jelly_king, struct jelly_king_definition* definition) {
    entity_id entity_id = entity_id_new();

    jelly_king->transform.position = definition->position;
    jelly_king->transform.rotation = definition->rotation;

    renderable_single_axis_init(&jelly_king->renderable, &jelly_king->transform, "rom:/meshes/enemies/jelly_king.tmesh");

    render_scene_add_renderable(&jelly_king->renderable, 3.0f);
    
    jelly_king->animation_set = animation_cache_load("rom:/meshes/enemies/jelly_king.anim");
    jelly_king->animations.idle = animation_set_find_clip(jelly_king->animation_set, "idle");
    jelly_king->animations.attack = animation_set_find_clip(jelly_king->animation_set, "attack");
    jelly_king->state = JELLY_KING_IDLE;

    animator_init(&jelly_king->animator, jelly_king->renderable.armature.bone_count);
    animator_run_clip(&jelly_king->animator, jelly_king->animations.idle, 0.0f, true);

    update_add(jelly_king, jelly_king_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);

    dynamic_object_init(
        entity_id,
        &jelly_king->collider,
        &jelly_king_collider,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY,
        &jelly_king->transform.position,
        &jelly_king->transform.rotation
    );
    jelly_king->collider.center.y = jelly_king_collider.data.cylinder.half_height;
    jelly_king->collider.collision_group = entity_id;

    collision_scene_add(&jelly_king->collider);

    health_init(&jelly_king->health, entity_id, MAX_HEALTH);

    spatial_trigger_init(&jelly_king->vision, &jelly_king->transform, &jelly_king_vision_type, COLLISION_LAYER_DAMAGE_PLAYER);
    collision_scene_add_trigger(&jelly_king->vision);

    vector2ComplexFromAngle(MAX_ROTATE_PER_SECOND * M_DEG_2_RAD * fixed_time_step, &jelly_king->max_rotate);
}

void jelly_king_destroy(struct jelly_king* jelly_king) {
    render_scene_remove(&jelly_king->renderable);
    renderable_destroy(&jelly_king->renderable);
    update_remove(jelly_king);
    collision_scene_remove(&jelly_king->collider);
    collision_scene_remove_trigger(&jelly_king->vision);
}