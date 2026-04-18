#include "biter.h"

#include "../collision/collision_scene.h"
#include "../collision/shapes/cone.h"
#include "../collision/shapes/sphere.h"
#include "../render/render_scene.h"
#include "../resource/animation_cache.h"
#include "../time/time.h"
#include "../math/constants.h"
#include "../entity/entity_spawner.h"
#include "vision.h"
#include "../math/mathf.h"

#define VISION_DISTANCE     8.0f
#define ATTACK_RANGE        1.0f
#define MOVE_SPEED          6.5f
#define MOVE_ACCELERATION   8.0f

static struct Vector2 biter_max_rotation;

static struct dynamic_object_type biter_collision_type = {
    .minkowsi_sum = sphere_minkowski_sum,
    .bounding_box = sphere_bounding_box,
    .data = {
        .sphere = {
            .radius = 0.4f,
        },
    },
    .friction = 0.1f,
    // about a 40 degree slope
    .max_stable_slope = 0.219131191f,
    .center = { 0.0f, 0.4f, 0.0f },
};

static struct spatial_trigger_type biter_vision_type = {
    .type = SPATIAL_TRIGGER_WEDGE,
    .data = {
        .wedge = {
            .radius = VISION_DISTANCE,
            .half_height = VISION_DISTANCE,
            .angle = {SQRT_1_2, SQRT_1_2},
        },
    },
};

void biter_enter_idle(struct biter* biter) {
    biter->current_state = BITER_STATE_IDLE;
    animator_run_clip(&biter->animator, biter->animations[BITER_ANIMATION_IDLE], 0, true);
}

void biter_enter_suprise(struct biter* biter) {
    biter->current_state = BITER_STATE_SEE;
    animator_run_clip(&biter->animator, biter->animations[BITER_ANIMATION_SEE], 0, false);
}

void biter_enter_chase(struct biter* biter) {
    biter->current_state = BITER_STATE_CHASE;
    animator_run_clip(&biter->animator, biter->animations[BITER_ANIMATION_WALK], 0, true);
    biter->rest_timer = randomInRangef(5.0f, 10.0f);
}

void biter_enter_rest(struct biter* biter) {
    biter->current_state = BITER_STATE_REST;
    animator_run_clip(&biter->animator, biter->animations[BITER_ANIMATION_REST], 0, false);
}

void biter_enter_trip(struct biter* biter) {
    biter->current_state = BITER_STATE_TRIP;
    animator_run_clip(&biter->animator, biter->animations[BITER_ANIMATION_TRIP], 0, false);
}

void biter_enter_lose_player(struct biter* biter) {
    biter->current_state = BITER_STATE_LOSE_PLAYER;
    animator_run_clip(&biter->animator, biter->animations[BITER_ANIMATION_CONFUSED], 0, false);
}

void biter_enter_attack(struct biter* biter) {
    biter->current_state = BITER_STATE_ATTACK;
    animator_run_clip(&biter->animator, biter->animations[BITER_ANIMATION_BITE], 0, false);
}

void biter_enter_death(struct biter* biter) {
    biter->current_state = BITER_STATE_DIE;
    animator_run_clip(&biter->animator, biter->animations[BITER_ANIMATION_DIE], 0, false);
}

void biter_update_idle(struct biter* biter) {
    struct Vector3 direction;
    struct dynamic_object* target_object = vision_update_current_target(
        &biter->current_target,
        &biter->vision,
        VISION_DISTANCE,
        &direction
    );

    if (target_object) {
        biter_enter_suprise(biter);
    }
}

void biter_update_see(struct biter* biter) {
    if (animator_is_running(&biter->animator)) {
        return;
    }

    
    struct Vector3 direction;
    struct dynamic_object* target_object = vision_update_current_target(
        &biter->current_target,
        &biter->vision,
        VISION_DISTANCE,
        &direction
    );

    if (target_object) {
        biter_enter_chase(biter);
    } else {
        biter_enter_lose_player(biter);
    }
}

void biter_update_chase(struct biter* biter) {
    struct Vector3 direction;
    struct dynamic_object* target_object = vision_update_current_target(
        &biter->current_target,
        &biter->vision,
        VISION_DISTANCE,
        &direction
    );

    if (!target_object) {
        biter_enter_lose_player(biter);
        return;
    }

    if (randomChance(0.002f)) {
        biter_enter_trip(biter);
        return;
    }

    biter->rest_timer -= fixed_time_step;

    if (biter->rest_timer <= 0.0f) {
        biter_enter_rest(biter);
    }

    struct Vector2 rotation;
    float distance_sqrd = vector3MagSqrd2D(&direction);
    bool should_attack = distance_sqrd < ATTACK_RANGE * ATTACK_RANGE || dynamic_object_find_contact(&biter->dynamic_object, biter->current_target);

    vector2LookDir(&rotation, &direction);

    float angle_dot = vector2Dot(&rotation, &biter->transform.rotation);

    if (should_attack && angle_dot > 0.9) {
        biter_enter_attack(biter);
        return;
    }

    vector2RotateTowards(&biter->transform.rotation, &rotation, &biter_max_rotation, &biter->transform.rotation);

    if (should_attack) {
        // stop moving to attack
        return;
    }

    if (angle_dot > 0.0f && !biter->dynamic_object.is_pushed) {
        struct Vector3 targetVelocity;
        vector3RotatedSpeed(&biter->transform.rotation, &targetVelocity, MOVE_SPEED);
        vector3MoveTowards(&biter->dynamic_object.velocity, &targetVelocity, MOVE_ACCELERATION * fixed_time_step, &biter->dynamic_object.velocity);
    }
}

void biter_update_rest(struct biter* biter) {
    if (animator_is_running(&biter->animator)) {
        return;
    }

    biter_enter_chase(biter);
}

void biter_update_trip(struct biter* biter) {
    if (animator_is_running(&biter->animator)) {
        return;
    }

    biter_enter_chase(biter);
}

void biter_update_lose_player(struct biter* biter) {
    if (animator_is_running(&biter->animator)) {
        return;
    }

    biter_enter_idle(biter);
}

void biter_update_attack(struct biter* biter) {
    if (!animator_is_running_clip(&biter->animator, biter->animations[BITER_ANIMATION_BITE])) {
        biter_enter_chase(biter);
        return;
    }

    if (!biter->current_target || !biter->animator.events.attack) {
        return;
    }

    struct dynamic_object* target_object = collision_scene_find_object(biter->current_target);

    if (!target_object) {
        return;
    }
    
    if (vector3DistSqrd(target_object->position, &biter->transform.position) > ATTACK_RANGE * ATTACK_RANGE) {
        return;
    }

    struct damage_info damage = {
        .amount = 1.0f,
        .type = DAMAGE_TYPE_BASH,
        .source = biter->dynamic_object.entity_id,
    };
    vector2ToLookDir(&biter->transform.rotation, &damage.direction);
    health_damage_id(biter->current_target, &damage, NULL);
    biter->current_target = 0;
}

void biter_update_die(struct biter* biter) {
    if (!animator_is_running(&biter->animator)) {
        entity_despawn(biter->health.entity_id);
    }
}

void biter_update(struct biter* biter) {
    animator_update(&biter->animator, fixed_time_step);

    switch (biter->current_state) {
        case BITER_STATE_IDLE:
            biter_update_idle(biter);
            break;
        case BITER_STATE_SEE:
            biter_update_see(biter);
            break;
        case BITER_STATE_CHASE:
            biter_update_chase(biter);
            break;
        case BITER_STATE_REST:
            biter_update_rest(biter);
            break;
        case BITER_STATE_TRIP:
            biter_update_trip(biter);
            break;
        case BITER_STATE_LOSE_PLAYER:
            biter_update_lose_player(biter);
            break;
        case BITER_STATE_ATTACK:
            biter_update_attack(biter);
            break;
        case BITER_STATE_DIE:
            biter_update_die(biter);
            break;
    }

    if (biter->health.current_health <= 0.0f && biter->current_state != BITER_STATE_DIE) {
        biter_enter_death(biter);
    }
}

static char* biter_animations[BITER_ANIMATION_COUNT] = {
    [BITER_ANIMATION_IDLE] = "enemy1_idle",
    [BITER_ANIMATION_SEE] = "enemy1_suprise",
    [BITER_ANIMATION_WALK] = "enemy1_walk",
    [BITER_ANIMATION_REST] = "enemy1_rest",
    [BITER_ANIMATION_TRIP] = "enemy1_trip",
    [BITER_ANIMATION_CONFUSED] = "enemy1_confused",
    [BITER_ANIMATION_BITE] = "enemy1_attack",
    [BITER_ANIMATION_DIE] = "enemy1_die",
};

void biter_init(struct biter* biter, struct biter_definition* definition, entity_id id) {
    transformSaInit(&biter->transform, &definition->position, &definition->rotation, 1.0f);

    renderable_single_axis_init(&biter->renderable, &biter->transform, "rom:/meshes/enemies/enemy1.tmesh");
    renderable_set_animator(&biter->renderable, &biter->animator);
    dynamic_object_init(
        id, 
        &biter->dynamic_object, 
        &biter_collision_type, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY | COLLISION_LAYER_Z_TARGET,
        &biter->transform.position, 
        &biter->transform.rotation
    );

    spatial_trigger_init(
        &biter->vision, 
        &biter->transform,
        &biter_vision_type,
        COLLISION_LAYER_DAMAGE_PLAYER,
        id
    );

    biter->dynamic_object.density_class = DYNAMIC_DENSITY_MEDIUM;

    collision_scene_add(&biter->dynamic_object);
    collision_scene_add_trigger(&biter->vision);

    update_add(biter, (update_callback)biter_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);

    render_scene_add_renderable(&biter->renderable, 1.73f);

    health_init(&biter->health, id, 10.0f);

    biter->animation_set = animation_cache_load("rom:/meshes/enemies/enemy1.anim");

    for (int i = 0; i < sizeof(biter_animations) / sizeof(biter_animations[0]); i += 1) {
        biter->animations[i] = animation_set_find_clip(biter->animation_set, biter_animations[i]);
    }

    biter->current_target = 0;
    biter->current_state = BITER_STATE_IDLE;
    biter->rest_timer = 0.0f;

    animator_init(&biter->animator, biter->renderable.mesh_render.armature.bone_count);

    vector2ComplexFromAngle(fixed_time_step * 3.0f, &biter_max_rotation);
    biter_enter_idle(biter);
}

void biter_destroy(struct biter* biter) {
    render_scene_remove(&biter->renderable);
    renderable_destroy(&biter->renderable);
    collision_scene_remove(&biter->dynamic_object);
    collision_scene_remove_trigger(&biter->vision);
    health_destroy(&biter->health);
    update_remove(biter);
    animator_destroy(&biter->animator);
    animation_cache_release(biter->animation_set);
}

void biter_common_init() {

}

void biter_common_destroy() {
    
}