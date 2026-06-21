#include "golem_enemy.h"    

#include "../math/constants.h"
#include "../collision/shapes/capsule.h"
#include "../collision/shapes/sphere.h"
#include "../math/mathf.h"

#define VISION_DISTANCE 8.0f

#define GOLEM_TURN_RATE             0.7f
#define GOLEM_HEAD_TURN_RATE        2.0f
#define GOLEM_HEAD_ROTATION_OFFSET  -0.78f
#define GOLEM_WALK_SPEED            0.8f
#define GOLEM_ACCEL                 0.5f

#define PUNCH_RANGE         2.0f

#define HEAD_BONE           1

static tmesh_t* golem_pot;
static tmesh_t* golem_full;
static armature_attachment_t* golem_r_attachment;

enum golem_animations {
    GOLEM_ANIM_WAKE_UP,
    GOLEM_ANIM_WALK,
    GOLEM_ANIM_PUNCH,

    GOLEM_ANIM_COUNT,
};

static animation_set_t* golem_animation_set;
static animation_clip_t* golem_animations[GOLEM_ANIM_COUNT];

static const char* golem_animation_names[GOLEM_ANIM_COUNT] = {
    [GOLEM_ANIM_WAKE_UP] = "wake_up",
    [GOLEM_ANIM_WALK] = "walk",
    [GOLEM_ANIM_PUNCH] = "punch",
};

static dynamic_object_type_t golem_collider = {
    CAPSULE_COLLIDER(0.7f, 0.8f),
    .center = {0.0f, 1.5f, 0.0f},
};

static dynamic_object_type_t golem_first_r_collider = {
    SPHERE_COLLIDER(1.0f),
};

static spatial_trigger_type_t golem_vision = {
    .type = SPATIAL_TRIGGER_WEDGE,
    .data = {
        .wedge = {
            .radius = VISION_DISTANCE,
            .half_height = VISION_DISTANCE,
            .angle = {SQRT_1_2, SQRT_1_2},
        },
    },
};

static struct damage_source punch_attack = {
    .amount = 10.0f,
    .type = DAMAGE_TYPE_KNOCKBACK,
    .knockback_strength = damage_knockback_with_time(1.0f),
};

static vector2_t golem_rotation_speed;
static vector2_t golem_head_rotation_speed;

void golem_enemy_look_forward(golem_enemy_t* golem) {
    vector2RotateTowards(&golem->head_rotation, &gRight2, &golem_head_rotation_speed, &golem->head_rotation);
}

void golem_look_at_target(golem_enemy_t* golem) {
    dynamic_object_t* target = collision_scene_find_object(golem->target);

    if (!target) {
        return;
    }

    vector3_t offset;
    vector3Sub(target->position, &golem->transform.position, &offset);

    vector2_t target_rotation;
    vector2_t inv_rot;
    vector2ComplexConj(&golem->transform.rotation, &inv_rot);
    vector2LookDir(&target_rotation, &offset);

    vector2ComplexMul(&inv_rot, &target_rotation, &target_rotation);

    vector2RotateTowards(&golem->head_rotation, &target_rotation, &golem_head_rotation_speed, &golem->head_rotation);
}

void golem_enemy_render(void* data, struct render_batch* batch) {
    golem_enemy_t* golem = (golem_enemy_t*)data;
    T3DMat4FP* mtx = render_batch_transformfp_from_sa(batch, &golem->transform);

    if (!mtx) {
        return;
    }

    if (golem->state == GOLEM_STATE_IDLE) {
        render_batch_add_tmesh(batch, golem_pot, mtx, NULL, NULL, NULL);
        return;
    }

    animator_apply(&golem->animator, &golem->renderable.mesh_render.armature);
    quatAxisComplex(
        &gUp, 
        &golem->head_rotation,
        &golem->renderable.mesh_render.armature.pose[HEAD_BONE].rotation
    );
    
    render_batch_add_tmesh(batch, golem_full, mtx, &golem->renderable.mesh_render.armature, NULL, NULL);
}

void golem_enemy_activate(golem_enemy_t* golem) {
    animator_run_clip(&golem->animator, golem_animations[GOLEM_ANIM_WAKE_UP], 0.0f, false);
    golem->state = GOLEM_STATE_ACTIVATING;
    golem->animator_speed = 1.0f;
    golem->collider.collision_layers |= COLLISION_LAYER_Z_TARGET;
}

void golem_enemy_follow(golem_enemy_t* golem) {
    animator_run_clip(&golem->animator, golem_animations[GOLEM_ANIM_WALK], 0.0f, true);
    golem->state = GOLEM_STATE_FOLLOW;
    golem->animator_speed = 0.0f;
}

void golem_enemy_punch(golem_enemy_t* golem) {
    animator_run_clip(&golem->animator, golem_animations[GOLEM_ANIM_PUNCH], 0.0f, false);
    golem->state = GOLEM_STATE_PUNCH;
    golem->animator_speed = 1.0f;
    golem->target_speed = 0.0f;
    golem->collider.velocity = gZeroVec;
    collision_scene_add(&golem->fist_r_collider);
}

void golem_enemy_deactivate(golem_enemy_t* golem) {
    animator_run_clip(
        &golem->animator, 
        golem_animations[GOLEM_ANIM_WAKE_UP], 
        animation_clip_get_duration(golem_animations[GOLEM_ANIM_WAKE_UP]), 
        false
    );
    golem->state = GOLEM_STATE_DEACTIVATE;
    golem->collider.collision_layers &= ~COLLISION_LAYER_Z_TARGET;
}

void golem_enemy_update_idle(golem_enemy_t* golem) {
    vector3_t offset;
    dynamic_object_t* target = vision_update_current_target(&golem->target, &golem->vision, VISION_DISTANCE, &offset);

    if (target) {
        golem_enemy_activate(golem);
    }
    
    golem_enemy_look_forward(golem);
}

void golem_enemy_update_activating(golem_enemy_t* golem) {
    if (!animator_is_running_clip(&golem->animator, golem_animations[GOLEM_ANIM_WAKE_UP])) {
        golem_enemy_follow(golem);
    }

    golem_enemy_look_forward(golem);
}

void golem_update_speed(golem_enemy_t* golem, float speed) {
    golem->target_speed = mathfMoveTowards(golem->target_speed, speed, GOLEM_ACCEL * fixed_time_step);

    vector3_t target_vel;
    vector2ToLookDir(&golem->transform.rotation, &target_vel);
    vector3Scale(&target_vel, &target_vel, golem->target_speed);
    target_vel.y = golem->collider.velocity.y;
    golem->collider.velocity = target_vel;
}

void golem_enemy_update_follow(golem_enemy_t* golem) {
    vector3_t offset;
    dynamic_object_t* target = vision_update_current_target(&golem->target, &golem->vision, VISION_DISTANCE, &offset);

    if (!target) {
        golem_enemy_deactivate(golem);
        return;
    }

    vector2_t target_rotate;
    vector2LookDir(&target_rotate, &offset);
    vector2RotateTowards(&golem->transform.rotation, &target_rotate, &golem_rotation_speed, &golem->transform.rotation);

    golem->animator_speed = sqrtf(vector3MagSqrd2D(&golem->collider.velocity)) * (1.0f / GOLEM_WALK_SPEED);
    golem_update_speed(golem, GOLEM_WALK_SPEED);

    if (vector3MagSqrd2D(&offset) < PUNCH_RANGE * PUNCH_RANGE) {
        golem_enemy_punch(golem);
    }

    golem_look_at_target(golem);
}

void golem_enemy_update_punch(golem_enemy_t* golem) {
    golem_update_speed(golem, 0.0f);

    animator_apply(&golem->animator, &golem->renderable.mesh_render.armature);

    transform_t fist_transform;
    armature_bone_transform(&golem->renderable.mesh_render.armature, golem_r_attachment->bone_index, &fist_transform);

    transformPoint(&fist_transform, &golem_r_attachment->local_pos, &golem->fist_r_position);
    vector3Scale(&golem->fist_r_position, &golem->fist_r_position, 1.0f / MODEL_SCALE);
    transformSaTransformPoint(&golem->transform, &golem->fist_r_position, &golem->fist_r_position);

    if (!animator_is_running_clip(&golem->animator, golem_animations[GOLEM_ANIM_PUNCH])) {
        collision_scene_remove(&golem->fist_r_collider);
        golem_enemy_follow(golem);
        return;
    }

    if (golem->animator.events.attack) {
        health_apply_contact_damage(golem->fist_r_collider.active_contacts, &punch_attack, NULL);
    }
    
    golem_enemy_look_forward(golem);
}

void golem_enemy_update_deactivate(golem_enemy_t* golem) {
    golem_update_speed(golem, 0.0f);

    if (!animator_is_running_clip(&golem->animator, golem_animations[GOLEM_ANIM_WAKE_UP])) {
        golem->state = GOLEM_STATE_IDLE;
    }
    
    golem_enemy_look_forward(golem);
}

void golem_enemy_update(void* data) {
    golem_enemy_t* golem_enemy = (golem_enemy_t*)data;

    animator_update(&golem_enemy->animator, fixed_time_step * golem_enemy->animator_speed);
    
    switch (golem_enemy->state) {
        case GOLEM_STATE_IDLE:
            golem_enemy_update_idle(golem_enemy);
            break;
        case GOLEM_STATE_ACTIVATING:
            golem_enemy_update_activating(golem_enemy);
            break;
        case GOLEM_STATE_FOLLOW:
            golem_enemy_update_follow(golem_enemy);
            break;
        case GOLEM_STATE_PUNCH:
            golem_enemy_update_punch(golem_enemy);
            break;
        case GOLEM_STATE_DEACTIVATE:
            golem_enemy_update_deactivate(golem_enemy);
            break;
    }
}

void golem_enemy_init(golem_enemy_t* golem_enemy, struct golem_enemy_definition* definition, entity_id entity_id) {
    transformSaInit(&golem_enemy->transform, &definition->position, &definition->rotation, 1.0f);

    renderable_single_axis_init_direct(&golem_enemy->renderable, &golem_enemy->transform, golem_full);
    render_scene_add(&golem_enemy->transform.position, 1.5f, golem_enemy_render, golem_enemy);
    animator_init(&golem_enemy->animator, golem_full->armature.bone_count);

    spatial_trigger_init(&golem_enemy->vision, &golem_enemy->transform, &golem_vision, COLLISION_LAYER_DAMAGE_PLAYER, entity_id);
    collision_scene_add_trigger(&golem_enemy->vision);

    dynamic_object_init(
        entity_id, 
        &golem_enemy->collider, 
        &golem_collider, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY, 
        &golem_enemy->transform.position, 
        &golem_enemy->transform.rotation
    );
    golem_enemy->collider.weight_class = WEIGHT_CLASS_HEAVY;
    collision_scene_add(&golem_enemy->collider);

    golem_enemy->activated = definition->activated;
    golem_enemy->target = 0;
    golem_enemy->state = GOLEM_STATE_IDLE;
    golem_enemy->animator_speed = 1.0f;
    golem_enemy->target_speed = 0.0f;

    golem_enemy->fist_r_position = gZeroVec;
    golem_enemy->head_rotation = gRight2;

    update_add(golem_enemy, golem_enemy_update, UPDATE_PRIORITY_ENEMY, UPDATE_LAYER_WORLD);

    dynamic_object_init(
        entity_id,
        &golem_enemy->fist_r_collider,
        &golem_first_r_collider,
        COLLISION_LAYER_DAMAGE_PLAYER,
        &golem_enemy->fist_r_position,
        NULL
    );
    golem_enemy->fist_r_collider.trigger_type = TRIGGER_TYPE_OVERLAP;
}

void golem_enemy_destroy(golem_enemy_t* golem_enemy, struct golem_enemy_definition* definition) {
    renderable_destroy_direct(&golem_enemy->renderable);
    render_scene_remove(&golem_enemy->renderable);
    update_remove(golem_enemy);
    collision_scene_remove_trigger(&golem_enemy->vision);
    collision_scene_remove(&golem_enemy->collider);

    if (golem_enemy->state == GOLEM_STATE_PUNCH) {
        collision_scene_remove(&golem_enemy->fist_r_collider);
    }
}

void golem_enemy_common_init() {
    golem_pot = tmesh_cache_load("rom:/meshes/enemies/golem_pot.tmesh");
    golem_full = tmesh_cache_load("rom:/meshes/enemies/golem.tmesh");

    golem_animation_set = animation_cache_load("rom:/meshes/enemies/golem.anim");

    for (int i = 0; i < GOLEM_ANIM_COUNT; i += 1) {
        golem_animations[i] = animation_set_find_clip(golem_animation_set, golem_animation_names[i]);
    }

    vector2ComplexFromAngle(GOLEM_TURN_RATE * fixed_time_step, &golem_rotation_speed);
    vector2ComplexFromAngle(GOLEM_HEAD_TURN_RATE * fixed_time_step, &golem_head_rotation_speed);

    golem_r_attachment = tmesh_find_attachment(golem_full, "fist_r");
}

void golem_enemy_common_destroy() {
    tmesh_cache_release(golem_pot);
    tmesh_cache_release(golem_full);
    animation_cache_release(golem_animation_set);
}
