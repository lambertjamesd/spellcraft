#include "tidal_wave.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"
#include "../entity/damage.h"
#include "../entity/health.h"

#define MOVE_SPEED  4.0f
#define MOVE_ACCEL  16.0f

#define MAX_VERTICAL_MOVEMENT   0.1f
#define WAVE_LIFETIME           5.0f
#define SCALE_UP_TIME           0.5f
#define SCALE_DOWN_TIME         3.0f

static spatial_trigger_type_t tidal_wave_trigger = {
    SPATIAL_TRIGGER_BOX(1.0f, 0.7f, 0.5f)
};

static damage_info_t tidal_wave_damage_source = {
    .amount = 0.1f,
    .type = DAMAGE_TYPE_WATER,
    .knockback_strength = 0.0f,
};

bool tidal_wave_move(tidal_wave_t* tidal_wave, float max_downward_movement) {
    struct Vector3 cast_from;
    
    vector2ToLookDir(&tidal_wave->transform.rotation, &cast_from);
    vector3Scale(&cast_from, &cast_from, MOVE_SPEED * fixed_time_step);
    vector3Add(&cast_from, &tidal_wave->transform.position, &cast_from);

    cast_from.y += MAX_VERTICAL_MOVEMENT;

    struct mesh_shadow_cast_result cast_result;

    if (!collision_scene_shadow_cast(&cast_from, &cast_result)) {
        return false;
    }

    if (cast_from.y - cast_result.y > max_downward_movement + MAX_VERTICAL_MOVEMENT) {
        return false;
    }

    tidal_wave->transform.position.x = cast_from.x;
    tidal_wave->transform.position.y = cast_result.y;
    tidal_wave->transform.position.z = cast_from.z;

    return true;
}

void tidal_wave_init(tidal_wave_t* tidal_wave, struct spell_data_source* source, struct spell_event_options event_options) {
    struct Vector2 rotation;
    vector2LookDir(&rotation, &source->direction);
    transformSaInit(&tidal_wave->transform, &source->position, &rotation, 0.0f);

    tidal_wave->max_scale = 2.0f;

    renderable_single_axis_init(&tidal_wave->renderable, &tidal_wave->transform, "rom:/meshes/spell/tidal_wave.tmesh");
    render_scene_add_renderable(&tidal_wave->renderable, 1.5f);

    spatial_trigger_init(&tidal_wave->trigger, &tidal_wave->transform, &tidal_wave_trigger, COLLISION_LAYER_DAMAGE_ENEMY);
    collision_scene_add_trigger(&tidal_wave->trigger);

    if (tidal_wave_move(tidal_wave, 2.0f)) {
        tidal_wave->timer = 0.0f;
    } else {
        tidal_wave->timer = WAVE_LIFETIME;
    }
}

void tidal_wave_destroy(tidal_wave_t* tidal_wave) {
    render_scene_remove(&tidal_wave->renderable);
    renderable_destroy(&tidal_wave->renderable);
    collision_scene_remove_trigger(&tidal_wave->trigger);
}

void tidal_wave_damage_and_push(tidal_wave_t* tidal_wave) {
    contact_t* contact = tidal_wave->trigger.active_contacts;

    struct Vector3 target_velocity;
    vector2ToLookDir(&tidal_wave->transform.rotation, &target_velocity);
    vector3Scale(&target_velocity, &target_velocity, MOVE_SPEED);
    target_velocity.y = 1.0f;

    while (contact) {
        dynamic_object_t* obj = collision_scene_find_object(contact->other_object);

        if (obj) {
            vector3MoveTowards(&obj->velocity, &target_velocity, MOVE_ACCEL, &obj->velocity);
        }

        health_damage_id(contact->other_object, &tidal_wave_damage_source, NULL); 

        contact = contact->next;
    }
}

bool tidal_wave_update(tidal_wave_t* tidal_wave) {
    if (!tidal_wave_move(tidal_wave, MAX_VERTICAL_MOVEMENT) && tidal_wave->timer < SCALE_DOWN_TIME) {
        tidal_wave->timer = SCALE_DOWN_TIME;
    }

    tidal_wave_damage_and_push(tidal_wave);

    float scale_factor;

    if (tidal_wave->timer < SCALE_UP_TIME) {
        scale_factor = 1.0f - tidal_wave->timer * (1.0f / SCALE_UP_TIME);
    } else if (tidal_wave->timer > SCALE_DOWN_TIME) {
        scale_factor = (tidal_wave->timer - SCALE_DOWN_TIME) * (1.0f / (WAVE_LIFETIME - SCALE_DOWN_TIME));
    } else {
        scale_factor = 0.0f;
    }

    scale_factor *= scale_factor;
    scale_factor = 1.0f - scale_factor;

    tidal_wave->transform.scale = tidal_wave->max_scale * scale_factor;

    tidal_wave->timer += fixed_time_step;
    return tidal_wave->timer < WAVE_LIFETIME;
}