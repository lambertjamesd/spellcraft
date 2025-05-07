#include "cutscene_actor.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../resource/animation_cache.h"
#include "../util/hash_map.h"
#include "../util/flags.h"

static struct hash_map cutscene_actor_hash_map;

#define SPACING_DISTANCE  1.8f

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

    animator_init(&actor->animator, armature ? armature->bone_count : 0);
    actor->armature = armature;
    actor->target = gZeroVec;
    actor->animate_speed = 1.0f;
    actor->state = ACTOR_STATE_IDLE;
    actor->id.npc_type = npc_type;
    actor->id.index = index;
    actor->def = def;

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

bool cutscene_actor_is_moving(struct cutscene_actor* actor) {
    return (actor->state & (ACTOR_STATE_LOOKING | ACTOR_STATE_MOVING | ACTOR_STATE_SPACE)) != 0;
}

bool cutscene_actor_update(struct cutscene_actor* actor) {
    animator_update(&actor->animator, actor->armature->pose, fixed_time_step * actor->animate_speed);

    if (actor->state == ACTOR_STATE_IDLE) {
        return false;
    }

    if (HAS_FLAG(actor->state, ACTOR_STATE_MOVING)) {
        if (vector3MoveTowards(
            &actor->transform.position, 
            &actor->target,
            actor->def->move_speed * fixed_time_step,
            &actor->transform.position
        )) {
            CLEAR_FLAG(actor->state, ACTOR_STATE_MOVING);
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
                vector3AddScaled(&actor->transform.position, &gRight, actor->def->move_speed * fixed_time_step, &actor->transform.position);
            } else {
                vector3AddScaled(&actor->transform.position, &offset, actor->def->move_speed * fixed_time_step / sqrtf(distSqrd), &actor->transform.position);
            }
        } else {
            CLEAR_FLAG(actor->state, ACTOR_STATE_SPACE);
        }
    }

    return true;
}