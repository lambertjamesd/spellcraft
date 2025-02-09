#include "living_sprite.h"

#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../render/defs.h"
#include "../collision/shapes/capsule.h"
#include "../collision/shapes/cylinder.h"
#include "../time/time.h"
#include "assets.h"

#define MIN_FOLLOW_DISTANCE 2.0f
#define TARGET_SPEED        12.0f
#define ACCELERATION        20.0f

static struct Vector2 sprite_max_rotation;

static struct dynamic_object_type living_sprite_collision = {
    .minkowsi_sum = capsule_minkowski_sum,
    .bounding_box = capsule_bounding_box,
    .data = {
        .capsule = {
            .radius = 0.25f,
            .inner_half_height = 0.1f,
        }
    },
    .friction = 0.5f,
    .bounce = 0.5f,
};

static struct dynamic_object_type living_sprite_vision = {
    .minkowsi_sum = cylinder_minkowski_sum,
    .bounding_box = cylinder_bounding_box,
    .data = {
        .cylinder = {
            .radius = 6.0f,
            .half_height = 2.0f,
        }
    },
};

void living_sprite_init(struct living_sprite* living_sprite, struct spell_data_source* source, struct spell_event_options event_options, struct living_sprite_definition* definition) {
    entity_id entity_id = entity_id_new();

    vector3AddScaled(&source->position, &source->direction, 0.5f, &living_sprite->transform.position);
    living_sprite->transform.rotation = gRight2;

    living_sprite->target = source->target;

    renderable_single_axis_init(&living_sprite->renderable, &living_sprite->transform, definition->model_file);

    render_scene_add_renderable(&living_sprite->renderable, 0.5f);

    dynamic_object_init(
        entity_id,
        &living_sprite->collider,
        &living_sprite_collision,
        COLLISION_LAYER_TANGIBLE,
        &living_sprite->transform.position,
        &living_sprite->transform.rotation
    );

    dynamic_object_init(
        entity_id,
        &living_sprite->vision,
        &living_sprite_vision,
        COLLISION_LAYER_DAMAGE_ENEMY,
        &living_sprite->transform.position,
        &living_sprite->transform.rotation
    );

    living_sprite->vision.center.y = living_sprite->collider.center.y;
    living_sprite->vision.is_trigger = true;

    living_sprite->collider.center.y = living_sprite_collision.data.capsule.radius + living_sprite_collision.data.capsule.inner_half_height;

    collision_scene_add(&living_sprite->collider);
    collision_scene_add(&living_sprite->vision);

    health_init(&living_sprite->health, entity_id, 10.0f);

    vector2ComplexFromAngle(fixed_time_step * 2.0f, &sprite_max_rotation);

    living_sprite->is_attacking = false;
    living_sprite->definition = definition;
}

void living_sprite_follow_target(struct living_sprite* living_sprite) {
    if (!living_sprite->target) {
        return;
    }

    struct dynamic_object* target = collision_scene_find_object(living_sprite->target);

    if (!target) {
        living_sprite->is_attacking = false;
        living_sprite->target = 0;
        return;
    }

    struct Vector3 offset;
    vector3Sub(target->position, &living_sprite->transform.position, &offset);
    offset.y = 0.0f;

    struct Vector3 dir;
    vector3Normalize(&offset, &dir);

    struct Vector2 targetRotation;
    vector2LookDir(&targetRotation, &dir);

    vector2RotateTowards(&living_sprite->transform.rotation, &targetRotation, &sprite_max_rotation, &living_sprite->transform.rotation);

    float distance = vector3Dot(&dir, &offset);

    if (!living_sprite->is_attacking && distance < MIN_FOLLOW_DISTANCE) {
        return;
    }

    float speed_ratio = vector2Dot(&targetRotation, &living_sprite->transform.rotation);
    struct Vector3 forward;
    vector2ToLookDir(&living_sprite->transform.rotation, &forward);

    struct Vector3 targetVelocity;
    vector3Scale(&forward, &targetVelocity, speed_ratio * TARGET_SPEED);
    targetVelocity.y = living_sprite->collider.velocity.y;

    vector3MoveTowards(&living_sprite->collider.velocity, &targetVelocity, fixed_time_step * ACCELERATION, &living_sprite->collider.velocity);
}

void living_sprite_check_targets(struct living_sprite* living_sprite) {
    if (living_sprite->is_attacking) {
        if (dynamic_object_is_touching(&living_sprite->collider, living_sprite->target)) {
            health_damage_id(living_sprite->target, living_sprite->definition->damage, living_sprite->collider.entity_id, living_sprite->definition->element_type);
            living_sprite->health.current_health = 0.0f;
        }
        return;
    }
    
    struct contact* new_target = dynamic_object_nearest_contact(&living_sprite->vision);

    if (!new_target || !new_target->other_object) {
        return;
    }

    living_sprite->target = new_target->other_object;
    living_sprite->is_attacking = true;
}

bool living_sprite_update(struct living_sprite* living_sprite, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    living_sprite_follow_target(living_sprite);
    living_sprite_check_targets(living_sprite);

    if (health_is_frozen(&living_sprite->health)) {
        living_sprite->health.current_health = 0.0f;
    }

    return health_is_alive(&living_sprite->health);
}

void living_sprite_destroy(struct living_sprite* living_sprite) {
    renderable_destroy(&living_sprite->renderable);
    render_scene_remove(&living_sprite->renderable);
    health_destroy(&living_sprite->health);
    collision_scene_remove(&living_sprite->collider);
    collision_scene_remove(&living_sprite->vision);
}