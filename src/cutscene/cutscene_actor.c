#include "cutscene_actor.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../resource/animation_cache.h"
#include "../util/hash_map.h"
#include "../util/flags.h"

static struct hash_map cutscene_actor_hash_map;

#define SPACING_DISTANCE        1.8f
#define AT_LOCATION_TOLERANCE   0.25f

void cutscene_actor_init(
    struct cutscene_actor* actor, 
    struct cutscene_actor_def* def,
    entity_id entity_id,
    struct TransformSingleAxis* transform,
    enum npc_type npc_type, 
    int index, 
    struct armature* armature, 
    char* animations_path
) {
    actor->transform = *transform;
    actor->animation_set = animations_path ? animation_cache_load(animations_path) : NULL;
    actor->animations.idle = animation_set_find_clip(actor->animation_set, "idle");
    actor->animations.walk = animation_set_find_clip(actor->animation_set, "walk");
    actor->animations.run = animation_set_find_clip(actor->animation_set, "run");

    animator_init(&actor->animator, armature ? armature->bone_count : 0);
    actor->armature = armature;
    actor->target = gZeroVec;
    actor->animate_speed = 1.0f;
    actor->state = ACTOR_STATE_IDLE;
    actor->id.npc_type = npc_type;
    actor->id.index = index;
    actor->def = def;
    actor->move_speed = def->move_speed;

    hash_map_set(&cutscene_actor_hash_map, actor->id.unique_id, actor);

    animator_run_clip(&actor->animator, actor->animations.idle, 0.0f, true);

    dynamic_object_init(
        entity_id,
        &actor->collider,
        &def->collider,
        def->collision_layers,
        &actor->transform.position,
        &actor->transform.rotation
    );

    actor->collider.center.y += def->half_height;
    actor->collider.collision_group = def->collision_group;

    collision_scene_add(&actor->collider);
}

void cutscene_actor_destroy(struct cutscene_actor* actor) {
    animation_cache_release(actor->animation_set);
    collision_scene_remove(&actor->collider);
    animator_destroy(&actor->animator);
    hash_map_delete(&cutscene_actor_hash_map, actor->id.unique_id);
}

void cutscene_actor_reset() {
    hash_map_destroy(&cutscene_actor_hash_map);
    hash_map_init(&cutscene_actor_hash_map, 32);
}

struct cutscene_actor* cutscene_actor_find(enum npc_type npc_type, int index) {
    cutscene_actor_id_t id;
    id.npc_type = npc_type;
    id.index = index;
    return hash_map_get(&cutscene_actor_hash_map, id.unique_id);
}

void cutscene_actor_interact_with(struct cutscene_actor* actor, enum interaction_type interaction, struct Vector3* at) {
    actor->target = *at;
    actor->state = interaction;
}

void cutscene_actor_idle(struct cutscene_actor* actor) {
    actor->state = ACTOR_STATE_IDLE;
}

void cutscene_actor_set_speed(struct cutscene_actor* actor, float speed) {
    actor->move_speed = speed;
}

bool cutscene_actor_is_moving(struct cutscene_actor* actor) {
    return (actor->state & (ACTOR_STATE_LOOKING | ACTOR_STATE_MOVING | ACTOR_STATE_SPACE | ACTOR_STATE_ANIMATING)) != 0;
}

bool cutscene_actor_update(struct cutscene_actor* actor) {
    animator_update(&actor->animator, actor->armature, fixed_time_step * actor->animate_speed);

    if (!cutscene_actor_is_moving(actor)) {
        return false;
    }

    if (HAS_FLAG(actor->state, ACTOR_STATE_MOVING)) {
        struct Vector3 offset;
        vector3Sub(&actor->target, &actor->transform.position, &offset);
        offset.y = 0.0f;
        float distSqrd = vector3MagSqrd(&offset);

        transform_rotate_towards(&actor->transform, &offset, actor->def->rotate_speed * fixed_time_step);

        if (distSqrd < AT_LOCATION_TOLERANCE * AT_LOCATION_TOLERANCE) {
            CLEAR_FLAG(actor->state, ACTOR_STATE_MOVING);
            actor->collider.velocity.x = 0.0f;
            actor->collider.velocity.z = 0.0f;
            if (!HAS_FLAG(actor->state, ACTOR_STATE_ANIMATING)) {
                animator_run_clip(&actor->animator, actor->animations.idle, 0.0f, true);
                actor->animate_speed = 1.0f;
            }
        } else {
            float inv = actor->move_speed / sqrtf(distSqrd);
            actor->collider.velocity.x = offset.x * inv;
            actor->collider.velocity.z = offset.z * inv;

            if (!HAS_FLAG(actor->state, ACTOR_STATE_ANIMATING)) {
                struct animation_clip* use_clip = actor->animations.walk;
                float target_move_speed = actor->def->move_speed;
    
                if (actor->move_speed > actor->def->run_threshold && actor->animations.run) {
                    use_clip = actor->animations.run;
                    target_move_speed = actor->def->run_speed;
                }
    
                if (!animator_is_running_clip(&actor->animator, use_clip)) {
                    animator_run_clip(&actor->animator, use_clip, 0.0f, true);
                    actor->animate_speed = actor->move_speed / target_move_speed;
                }
            }
        }
    }

    if (HAS_FLAG(actor->state, ACTOR_STATE_LOOKING)) {
        struct Vector3 offset;
        vector3Sub(&actor->target, &actor->transform.position, &offset);

        if (transform_rotate_towards(&actor->transform, &offset, actor->def->rotate_speed * fixed_time_step)) {
            CLEAR_FLAG(actor->state, ACTOR_STATE_LOOKING);
        }
    }

    if (HAS_FLAG(actor->state, ACTOR_STATE_SPACE)) {
        struct Vector3 offset;
        offset.y = 0.0f;
        vector3Sub(&actor->transform.position, &actor->target, &offset);

        float distSqrd = vector3MagSqrd(&offset);

        if (distSqrd < SPACING_DISTANCE * SPACING_DISTANCE) {
            if (distSqrd < 0.000001f) {
                vector3AddScaled(&actor->transform.position, &gRight, actor->move_speed * fixed_time_step, &actor->transform.position);
            } else {
                vector3AddScaled(&actor->transform.position, &offset, actor->move_speed * fixed_time_step / sqrtf(distSqrd), &actor->transform.position);
            }
        } else {
            CLEAR_FLAG(actor->state, ACTOR_STATE_SPACE);
        }
    }

    if (HAS_FLAG(actor->state, ACTOR_STATE_ANIMATING)) {
        if (!animator_is_running(&actor->animator)) {
            CLEAR_FLAG(actor->state, ACTOR_STATE_ANIMATING);
        }
    }

    return true;
}

void cutscene_actor_run_animation(struct cutscene_actor* actor, const char* name, bool loop) {
    if (*name == '\0') {
        CLEAR_FLAG(actor->state, ACTOR_STATE_ANIMATING);
        return;
    }

    animation_clip_t* clip = animation_set_find_clip(actor->animation_set, name);

    if (!clip) {
        return;
    }

    animator_run_clip(&actor->animator, clip, 0.0f, loop);
    actor->animate_speed = 1.0f;
    SET_FLAG(actor->state, ACTOR_STATE_ANIMATING);
}