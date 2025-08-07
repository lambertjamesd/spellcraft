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

#define MINION_DELAY_TIMER      10

#define DEFAULT_LAUNCH_SPEED    6.0f

#define ATTACK_DELAY_FRAMES     (5 * 50)

#define BITE_ATTACK_RANGE       3.0f

#define MAX_CHANGE_TIME         4.0f

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

static struct damage_source aeo_attack = {
    .amount = 10.0f,
    .type = DAMAGE_TYPE_KNOCKBACK,
    .knockback_strength = 2.0f,
};

bool jelly_king_rotate_to_target(struct jelly_king* jelly_king, struct contact* nearest_target, float threshold) {
    if (!nearest_target) {
        return false;
    }

    struct Vector3 offset;
    vector3Sub(&nearest_target->point, &jelly_king->transform.position, &offset);

    struct Vector2 targetRotation;
    vector2LookDir(&targetRotation, &offset);

    vector2RotateTowards(&jelly_king->transform.rotation, &targetRotation, &jelly_king->max_rotate, &jelly_king->transform.rotation);

    return vector2Dot(&targetRotation, &jelly_king->transform.rotation) > threshold;
}

bool jelly_king_can_fire_minion(struct jelly_king* jelly_king) {
    for (int i = 0; i < MAX_JELLY_MINIONS; i += 1) {
        if (!jelly_get_is_active(&jelly_king->minion[i])) {
            return true;
        }
    }

    return false;
}

void jelly_king_fire_minions(struct jelly_king* jelly_king, int count) {
    jelly_king->state_data.fire_minion.number_left = count;
    jelly_king->state = JELLY_KING_ATTACK_AIMING;
}

void jelly_king_start_idle(struct jelly_king* jelly_king) {
    jelly_king->state = JELLY_KING_IDLE;
    jelly_king->state_data.idle.attack_timer = ATTACK_DELAY_FRAMES;
    animator_run_clip(&jelly_king->animator, jelly_king->animations.idle, 0.0f, true);
}

void jelly_king_chase_player(struct jelly_king* jelly_king) {
    jelly_king->state = JELLY_KING_MOVE_TO_TARGET;
    jelly_king->state_data.chase.chase_timeout = MAX_CHANGE_TIME;
}

void jelly_king_start_aeo_attack(struct jelly_king* jelly_king) {
    jelly_king->state = JELLY_KING_ATTACK_AEO;
    animator_run_clip(&jelly_king->animator, jelly_king->animations.attack_aeo, 0.0f, false);
}

void jelly_move_towards_target(struct jelly_king* jelly_king, struct contact* nearest_target) {
    if (jelly_king_rotate_to_target(jelly_king, nearest_target, 0.5f)) {
        struct Vector3 targetVel;
        vector2ToLookDir(&jelly_king->transform.rotation, &targetVel);
        vector3Scale(&targetVel, &targetVel, SPEED);

        vector3MoveTowards(&jelly_king->collider.velocity, &targetVel, ACCEL * fixed_time_step, &jelly_king->collider.velocity);
    }
}

void jelly_king_idle(struct jelly_king* jelly_king) {
    struct contact* nearest_target = dynamic_object_nearest_contact(jelly_king->vision.active_contacts, &jelly_king->transform.position);

    if (!nearest_target) {
        return;
    }
    
    jelly_move_towards_target(jelly_king, nearest_target);

    if (jelly_king->state_data.idle.attack_timer > 0) {
        --jelly_king->state_data.idle.attack_timer;
    }

    if (jelly_king->state_data.idle.attack_timer <= 0) {
        struct Vector3 offset;
        vector3Sub(&nearest_target->point, &jelly_king->transform.position, &offset);

        float distanceSq = vector3MagSqrd2D(&offset);

        if (distanceSq < BITE_ATTACK_RANGE * BITE_ATTACK_RANGE) {
            struct Vector2 rotation;
            vector2LookDir(&rotation, &offset);

            if (vector2Dot(&rotation, &jelly_king->transform.rotation) < 0.7f) {
                jelly_king_start_aeo_attack(jelly_king);
            } else {
                jelly_king_chase_player(jelly_king);
            }
        } else if (jelly_king_can_fire_minion(jelly_king)) {
            jelly_king_fire_minions(jelly_king, 3);
        }
    }
}

void jelly_king_move_to_target(struct jelly_king* jelly_king) {
    struct contact* nearest_target = dynamic_object_nearest_contact(jelly_king->vision.active_contacts, &jelly_king->transform.position);

    if (!nearest_target) {
        return;
    }
    
    jelly_move_towards_target(jelly_king, nearest_target);

    if (dynamic_object_find_contact(&jelly_king->collider, nearest_target->other_object)) {
        animator_run_clip(&jelly_king->animator, jelly_king->animations.attack, 0.0f, false);
        jelly_king->state = JELLY_KING_ATTACK;
    }

    jelly_king->state_data.chase.chase_timeout -= fixed_time_step;

    if (jelly_king->state_data.chase.chase_timeout < 0.0f) {
        jelly_king_start_idle(jelly_king);
    }
}

void jelly_king_attack(struct jelly_king* jelly_king) {
    if (!animator_is_running(&jelly_king->animator)) {
        jelly_king_start_idle(jelly_king);
    }
}

void jelly_king_fire_jelly(struct jelly_king* jelly_king) {
    struct jelly* jelly;

    for (int i = 0; i < MAX_JELLY_MINIONS; i += 1) {
        jelly = &jelly_king->minion[jelly_king->next_minion];
        jelly_king->last_minion = jelly_king->next_minion;
        jelly_king->next_minion = (jelly_king->next_minion == MAX_JELLY_MINIONS - 1) ? 0 : (jelly_king->next_minion + 1);

        if (jelly_get_is_active(jelly)) {
            jelly = NULL;
            continue;
        }

        break;
    }

    if (!jelly) {
        return;
    }

    struct jelly_definition definition;
    definition.position = jelly_king->transform.position;
    definition.rotation = jelly_king->transform.rotation;
    jelly_init(jelly, &definition, entity_id_new());

    struct contact* nearest_target = dynamic_object_nearest_contact(jelly_king->vision.active_contacts, &jelly_king->transform.position);

    struct Vector3 attack_velocity;

    if (nearest_target) {
        // 0 = s * t + 1/2 * g * t * t
        // d = s * t

        // t = (d / s)

        // 0 = s * (d / s) + 0.5 * g * d*d/(s*s)
        // 0 = d + 0.5 * g * d*d/(s*s)
        // -d = 0.5 * g * d*d/(s*s)
        // s*s = -0.5 * g * d
        // s = sqrt(-0.5 * g * d)


        // s = sqrt(-g * 0.5 * distance)
        // vh = offset * s / distance
        // vv = s
        
        struct Vector3 offset;
        vector3Sub(&nearest_target->point, &jelly_king->transform.position, &offset);
        float distance = sqrtf(vector3MagSqrd2D(&offset));

        if (distance < 0.01f) {
            attack_velocity = gZeroVec;
        } else {
            float speed = sqrtf((-GRAVITY_CONSTANT * 0.5f) * distance);
            float distance_inv = speed / distance;
            
            attack_velocity.x = offset.x * distance_inv;
            attack_velocity.y = speed;
            attack_velocity.z = offset.z * distance_inv;
        }

    } else {
        vector2ToLookDir(&jelly_king->transform.rotation, &attack_velocity);
        vector3Scale(&attack_velocity, &attack_velocity, DEFAULT_LAUNCH_SPEED);
        attack_velocity.y = DEFAULT_LAUNCH_SPEED;
    }
    
    jelly_launch_attack(
        jelly, 
        &attack_velocity, 
        jelly_king->collider.collision_group, 
        nearest_target ? nearest_target->other_object : 0
    );
}

void jelly_king_reaim(struct jelly_king* jelly_king) {
    --jelly_king->state_data.fire_minion.number_left;
    jelly_king->state = JELLY_KING_ATTACK_AIMING;
}

void jelly_king_attack_ranged(struct jelly_king* jelly_king) {
    if (!animator_is_running_clip(&jelly_king->animator, jelly_king->animations.attack_ranged)) {
        jelly_king_reaim(jelly_king);
        return;
    }

    if (jelly_king->animator.events) {
        jelly_king_fire_jelly(jelly_king);
        jelly_king_reaim(jelly_king);
    }
}

void jelly_king_attack_aiming(struct jelly_king* jelly_king) {
    if (animator_is_running_clip(&jelly_king->animator, jelly_king->animations.attack_ranged)) {
        return;
    } else if (!animator_is_running(&jelly_king->animator)) {
        struct jelly* jelly = &jelly_king->minion[jelly_king->last_minion];
        if (jelly_get_is_active(jelly)) {
            jelly_reset_collision_group(jelly);
        }
        animator_run_clip(&jelly_king->animator, jelly_king->animations.idle, 0.0f, true);
    }

    if (jelly_king->state_data.fire_minion.number_left == 0 || !jelly_king_can_fire_minion(jelly_king)) {
        jelly_king_start_idle(jelly_king);
        return;
    }

    struct contact* nearest_target = dynamic_object_nearest_contact(jelly_king->vision.active_contacts, &jelly_king->transform.position);

    if (!nearest_target) {
        return;
    }
    
    if (jelly_king_rotate_to_target(jelly_king, nearest_target, 0.95f)) {
        jelly_king->state = JELLY_KING_ATTACK_RANGED;
        animator_run_clip(&jelly_king->animator, jelly_king->animations.attack_ranged, 0.0f, false);
    }
}

void jelly_king_attack_aeo(struct jelly_king* jelly_king) {
    if (!animator_is_running(&jelly_king->animator)) {
        jelly_king_start_idle(jelly_king);
    }

    if (jelly_king->animator.events) {
        health_apply_contact_damage(&jelly_king->collider, &aeo_attack, NULL);
    }
}

void jelly_king_update(void* data) {
    struct jelly_king* jelly_king = (struct jelly_king*)data;
    animator_update(&jelly_king->animator, &jelly_king->renderable.armature, fixed_time_step);

    jelly_king->collider_type.data.cylinder.half_height = jelly_king_collider.data.cylinder.half_height * jelly_king->renderable.armature.pose[0].scale.y;
    jelly_king->collider.center.y = jelly_king->collider_type.data.cylinder.half_height;
    jelly_king->collider_type.data.cylinder.radius = jelly_king_collider.data.cylinder.radius * jelly_king->renderable.armature.pose[0].scale.x;

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
        case JELLY_KING_ATTACK_RANGED:
            jelly_king_attack_ranged(jelly_king);
            break;
        case JELLY_KING_ATTACK_AIMING:
            jelly_king_attack_aiming(jelly_king);
            break;
        case JELLY_KING_ATTACK_AEO:
            jelly_king_attack_aeo(jelly_king);
            break;
    }
}

void jelly_king_init(struct jelly_king* jelly_king, struct jelly_king_definition* definition, entity_id id) {
    transformSaInit(&jelly_king->transform, &definition->position, &definition->rotation, 1.0f);

    renderable_single_axis_init(&jelly_king->renderable, &jelly_king->transform, "rom:/meshes/enemies/jelly_king.tmesh");

    render_scene_add_renderable(&jelly_king->renderable, 3.0f);
    
    jelly_king->animation_set = animation_cache_load("rom:/meshes/enemies/jelly_king.anim");
    jelly_king->animations.idle = animation_set_find_clip(jelly_king->animation_set, "idle");
    jelly_king->animations.attack = animation_set_find_clip(jelly_king->animation_set, "attack");
    jelly_king->animations.attack_ranged = animation_set_find_clip(jelly_king->animation_set, "attack_ranged");
    jelly_king->animations.attack_aeo = animation_set_find_clip(jelly_king->animation_set, "attack_aeo");

    animator_init(&jelly_king->animator, jelly_king->renderable.armature.bone_count);
    jelly_king_start_idle(jelly_king);

    update_add(jelly_king, jelly_king_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);

    jelly_king->collider_type = jelly_king_collider;

    dynamic_object_init(
        id,
        &jelly_king->collider,
        &jelly_king->collider_type,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY,
        &jelly_king->transform.position,
        &jelly_king->transform.rotation
    );
    jelly_king->collider.center.y = jelly_king_collider.data.cylinder.half_height;
    jelly_king->collider.collision_group = id;
    jelly_king->next_minion = 0;
    jelly_king->last_minion = 0;

    collision_scene_add(&jelly_king->collider);

    health_init(&jelly_king->health, id, MAX_HEALTH);

    spatial_trigger_init(&jelly_king->vision, &jelly_king->transform, &jelly_king_vision_type, COLLISION_LAYER_DAMAGE_PLAYER);
    collision_scene_add_trigger(&jelly_king->vision);

    vector2ComplexFromAngle(MAX_ROTATE_PER_SECOND * M_DEG_2_RAD * fixed_time_step, &jelly_king->max_rotate);

    memset(jelly_king->minion, 0, sizeof(jelly_king->minion));
}

void jelly_king_destroy(struct jelly_king* jelly_king) {
    render_scene_remove(&jelly_king->renderable);
    renderable_destroy(&jelly_king->renderable);
    update_remove(jelly_king);
    collision_scene_remove(&jelly_king->collider);
    collision_scene_remove_trigger(&jelly_king->vision);

    for (int i = 0; i < MAX_JELLY_MINIONS; i += 1) {
        jelly_destroy(&jelly_king->minion[i]);
    }
}