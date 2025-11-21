#include "grab_checker.h"

#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include <stddef.h>
#include <math.h>

#include "../resource/tmesh_cache.h"

#define DEBUG_GRABBER   1

#if DEBUG_GRABBER
static tmesh_t* grab_checker_mesh; 
#endif

#define HORIZONTAL_TOLERANCE    0.1f
#define WALL_CHECK_TOLERANCE    0.701f
#define MAX_GRAB_HEIGHT         2.2f
#define GROUND_LEVEL_TOLERANCE  0.5f
#define GRAB_TIMER_THRESHOLD    15
#define MAX_VERTICAL_DELTA      0.01f

#define CLIMB_OFFSET            0.5f

#if DEBUG_GRABBER

void grab_checker_render(void* data, struct render_batch* batch) {
    grab_checker_t* checker = (grab_checker_t*)data;

    if (!checker->can_grab) {
        return;
    }

    transform_sa_t transform = {
        .position = checker->climb_to,
        .rotation = gRight2,
        .scale = 1.0f,
    };

    T3DMat4FP* mtx = render_batch_transformfp_from_sa(batch, &transform);
    render_batch_add_tmesh(batch, grab_checker_mesh, mtx, NULL, NULL, NULL);
}

#endif

void grab_checker_init(grab_checker_t* checker, struct dynamic_object_type* collider_type) {
    dynamic_object_init(
        entity_id_new(),
        &checker->collider,
        collider_type,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE,
        &checker->position, 
        NULL
    );
    checker->collider.weight_class = WEIGHT_CLASS_GHOST;
    checker->collider.center.y = collider_type->data.capsule.inner_half_height + collider_type->data.capsule.radius;
    checker->collider.collision_group = ENTITY_ID_PLAYER;
    checker->position = gZeroVec;
    collision_scene_add(&checker->collider);
    checker->can_grab = false;
    checker->did_cast = false;
    checker->grab_timer = 0;
    checker->last_player_y = 0.0f;

#if DEBUG_GRABBER
    if (!grab_checker_mesh) {
        grab_checker_mesh = tmesh_cache_load("rom:/meshes/player/z_cursor.tmesh");
    }
    render_scene_add(&checker->climb_to, 0.5f, grab_checker_render, checker);
#endif
}

bool grab_checker_check_for_grab(grab_checker_t* checker) {
    if (!checker->did_cast) {
        return false;
    }

    contact_t* ground = dynamic_object_get_ground(&checker->collider);

    struct Vector2 offset = {
        checker->target_pos.x - checker->position.x,
        checker->target_pos.y - checker->position.z,
    };
    
    if (!ground || ground->normal.y <= GROUND_LEVEL_TOLERANCE || vector2MagSqr(&offset) >= 0.1f) {
        return false;
    }

    contact_t* curr = checker->collider.active_contacts;

    while (curr) {
        if (fabsf(curr->normal.y) < 0.5f && offset.x * curr->normal.x + offset.y * curr->normal.z < 0.0f) {
            return false;
        }

        curr = curr->next;
    }

    checker->climb_to = (struct Vector3){
        .x = checker->target_pos.x,
        .y = checker->position.y,
        .z = checker->target_pos.y,
    };
    ++checker->grab_timer;
    return true;
}

bool grab_checker_update(grab_checker_t* checker, dynamic_object_t* player_collider, struct Vector3* target_direction) {
    checker->can_grab = grab_checker_check_for_grab(checker);
    
    contact_t* wall_contact = NULL;
    float best_wall_tolernace = 0.0f;

    float player_y = player_collider->position->y;
    float delta = fabsf(player_y - checker->last_player_y);
    checker->last_player_y = player_y;

    if (delta > MAX_VERTICAL_DELTA) {
        return false;
    }
    
    for (
        contact_t* curr = player_collider->active_contacts;
        curr;
        curr = curr->next
    ) {
        if (fabsf(curr->normal.y) > HORIZONTAL_TOLERANCE) {
            continue;
        }
        
        float tolerance = curr->normal.x * target_direction->x + curr->normal.z * target_direction->z;

        if (tolerance > -WALL_CHECK_TOLERANCE || tolerance > best_wall_tolernace) {
            continue;
        }

        wall_contact = curr;
        best_wall_tolernace = tolerance;
    }

    if (!wall_contact) {
        checker->did_cast = false;
        checker->grab_timer = 0;
        return false;
    }

    checker->did_cast = true;

    struct Vector3 cast_from = {
        .x = player_collider->position->x - wall_contact->normal.x * CLIMB_OFFSET,
        .y = player_collider->position->y + MAX_GRAB_HEIGHT,
        .z = player_collider->position->z - wall_contact->normal.z * CLIMB_OFFSET,
    };

    checker->target_pos = (struct Vector2){.x = cast_from.x, .y = cast_from.z};

    if (checker->can_grab) {
        checker->collider.position->x = cast_from.x;
        checker->collider.position->z = cast_from.z;
        checker->collider.velocity = (struct Vector3){0.0f, -1.0f, 0.0f};
    } else {
        *checker->collider.position = cast_from;
        checker->collider.velocity = (struct Vector3){0.0f, -MAX_GRAB_HEIGHT / fixed_time_step, 0.0f};
        checker->grab_timer = 0;
    }

    return checker->grab_timer > GRAB_TIMER_THRESHOLD;
}

void grab_checker_destroy(grab_checker_t* checker) {
    collision_scene_remove(&checker->collider);
#if DEBUG_GRABBER
    render_scene_remove(checker);
#endif
}

void grab_checker_get_climb_to(grab_checker_t* checker, struct Vector3* out) {
    *out = checker->climb_to;
}