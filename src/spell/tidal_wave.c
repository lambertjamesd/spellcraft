#include "tidal_wave.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"

#define MOVE_SPEED  2.0f

#define MAX_VERTICAL_MOVEMENT   0.1f

bool tidal_wave_move(tidal_wave_t* tidal_wave, float max_downward_movement) {
    struct Vector3 cast_from;
    
    vector2ToLookDir(&tidal_wave->transform.rotation, &cast_from);
    vector3Scale(&cast_from, &cast_from, MOVE_SPEED * fixed_time_step);
    vector3Add(&cast_from, &tidal_wave->transform.position, &cast_from);

    cast_from.y += MAX_VERTICAL_MOVEMENT;

    struct mesh_shadow_cast_result cast_result;

    if (!collision_scene_shadow_cast(&cast_from, &cast_result)) {
        tidal_wave->transform.position.x = cast_from.x;
        tidal_wave->transform.position.z = cast_from.z;
        return true;
    }

    if (cast_from.y - cast_result.y > max_downward_movement) {
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
    transformSaInit(&tidal_wave->transform, &source->position, &rotation, 1.0f);

    renderable_single_axis_init(&tidal_wave->renderable, &tidal_wave->transform, "rom:/meshes/spell/tidal_wave.tmesh");
    render_scene_add_renderable(&tidal_wave->renderable, 1.5f);

    if (tidal_wave_move(tidal_wave, 2.0f)) {
        tidal_wave->timer = 5.0f;
    } else {
        tidal_wave->timer = 0.0f;
    }
}

void tidal_wave_destroy(tidal_wave_t* tidal_wave) {
    render_scene_remove(&tidal_wave->renderable);
    renderable_destroy(&tidal_wave->renderable);
}

bool tidal_wave_update(tidal_wave_t* tidal_wave) {
    tidal_wave_move(tidal_wave, MAX_VERTICAL_MOVEMENT);

    tidal_wave->timer -= fixed_time_step;
    return tidal_wave->timer > 0.0f;
}