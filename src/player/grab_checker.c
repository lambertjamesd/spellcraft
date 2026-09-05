#include "grab_checker.h"

#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include <stddef.h>
#include <math.h>
#include "../scene/scene_definition.h"

#include "../resource/tmesh_cache.h"

#define DEBUG_GRABBER   0

#if DEBUG_GRABBER
static tmesh_t* grab_checker_mesh; 
#endif

#define HORIZONTAL_TOLERANCE    0.1f
#define WALL_CHECK_TOLERANCE    0.701f
#define GROUND_LEVEL_TOLERANCE  0.5f
#define GRAB_TIMER_THRESHOLD    0

#define HANG_OFFSET             0.3f
#define HANG_OVERHANG           0.1f
#define HANG_CLEARANCE          1.0f

#define CLIMB_OFFSET            0.5f

#if DEBUG_GRABBER

void grab_checker_render(void* data, struct render_batch* batch) {
    grab_checker_t* checker = (grab_checker_t*)data;

    if (!checker->grab_mode) {
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
    checker->position = gZeroVec;
    dynamic_object_init(
        entity_id_new(),
        &checker->collider,
        collider_type,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE,
        &checker->position, 
        NULL
    );
    checker->collider.weight_class = WEIGHT_CLASS_GHOST;
    checker->collider.collision_group = ENTITY_ID_PLAYER;
    checker->position = gZeroVec;
    collision_scene_add(&checker->collider);
    checker->grab_mode = GRAB_MODE_NONE;
    checker->cast_mode = GRAB_MODE_NONE;
    checker->target_pos = gZeroVec;
    checker->target_rot = gZeroVec2;

#if DEBUG_GRABBER
    if (!grab_checker_mesh) {
        grab_checker_mesh = tmesh_cache_load("rom:/meshes/player/z_cursor.tmesh");
    }
    render_scene_add(&checker->climb_to, 0.5f, grab_checker_render, checker);
#endif
}

grab_mode_t grab_checker_check_for_grab(grab_checker_t* checker) {
    switch (checker->cast_mode) {
        case GRAB_MODE_CLIMB: {
            contact_t* ground = dynamic_object_get_ground(&checker->collider);

            struct Vector2 offset = {
                checker->target_pos.x - checker->position.x,
                checker->target_pos.z - checker->position.z,
            };
            
            if (!ground || ground->normal.y <= GROUND_LEVEL_TOLERANCE || vector2MagSqr(&offset) >= 0.1f) {
                return GRAB_MODE_NONE;
            }

            contact_t* curr = checker->collider.active_contacts;

            while (curr) {
                if (fabsf(curr->normal.y) < 0.5f && offset.x * curr->normal.x + offset.y * curr->normal.z < 0.0f) {
                    return GRAB_MODE_NONE;
                }

                curr = curr->next;
            }

            checker->climb_to = (struct Vector3){
                .x = checker->target_pos.x,
                .y = checker->position.y,
                .z = checker->target_pos.z,
            };

            return GRAB_MODE_CLIMB;
        }
        case GRAB_MODE_HANG:
            if (checker->collider.active_contacts) {
                return GRAB_MODE_NONE;
            }
            
            checker->climb_to = checker->target_pos;
            return GRAB_MODE_HANG;
        default:
            return GRAB_MODE_NONE;
    }
}

void grab_checker_cast_climb(grab_checker_t* checker, dynamic_object_t* player_collider, contact_t* wall_contact, float max_grab_height) {
    checker->cast_mode = GRAB_MODE_CLIMB;

    checker->position = (vector3_t){
        .x = player_collider->position->x - wall_contact->normal.x * CLIMB_OFFSET,
        .y = player_collider->position->y + max_grab_height,
        .z = player_collider->position->z - wall_contact->normal.z * CLIMB_OFFSET,
    };
    checker->collider.velocity = (struct Vector3){0.0f, -max_grab_height / fixed_time_step, 0.0f};
    checker->target_pos = checker->position;
    dynamic_object_wake(&checker->collider);
}

void grab_checker_cast_hang(grab_checker_t* checker, dynamic_object_t* player_collider, vector3_t* target_direction, contact_t* ground_contact) {
    if (player_collider->velocity.y > 0.0f) {
        checker->cast_mode = GRAB_MODE_NONE;
        checker->position = gZeroVec;
        checker->target_pos = gZeroVec;
        return;
    }
    
    vector3_t cast_dir;
    if (ground_contact->normal.y > 0.95f) {
        cast_dir = *target_direction;
    } else {
        cast_dir = ground_contact->normal;
        cast_dir.y = 0.0f;
    }

    vector3Normalize(&cast_dir, &cast_dir);

    if (cast_dir.x == 0.0f && cast_dir.z == 0.0f) {
        checker->cast_mode = GRAB_MODE_NONE;
        checker->position = gZeroVec;
        checker->target_pos = gZeroVec;
        return;
    }

    checker->target_rot = (vector2_t){cast_dir.z, -cast_dir.x};

    cast_dir.x *= HANG_OFFSET;
    cast_dir.z *= HANG_OFFSET;
    cast_dir.y = HANG_OVERHANG;

    checker->cast_mode = GRAB_MODE_HANG;
    vector3Add(&ground_contact->point, &cast_dir, &checker->position);
    checker->collider.velocity = (struct Vector3){0.0f, -(HANG_OVERHANG + HANG_CLEARANCE) / fixed_time_step, 0.0f};
    checker->target_pos = ground_contact->point;
    dynamic_object_wake(&checker->collider);
}

grab_mode_t grab_checker_update(grab_checker_t* checker, dynamic_object_t* player_collider, struct Vector3* target_direction, float max_grab_height) {
    checker->grab_mode = grab_checker_check_for_grab(checker);
    
    contact_t* wall_contact = NULL;
    float best_wall_tolernace = 0.0f;

    contact_t* ground_contact = NULL;
    float best_ground_tolerance = 0.0f;
    
    for (
        contact_t* curr = player_collider->active_contacts;
        curr;
        curr = curr->next
    ) {
        if (fabsf(curr->normal.y) > HORIZONTAL_TOLERANCE) {
            if (curr->normal.y > best_ground_tolerance) {
                ground_contact = curr;
                best_ground_tolerance = curr->normal.y;
            }
        } else {
            float tolerance = curr->normal.x * target_direction->x + curr->normal.z * target_direction->z;
    
            if (tolerance > -WALL_CHECK_TOLERANCE || tolerance > best_wall_tolernace) {
                continue;
            }
    
            wall_contact = curr;
            best_wall_tolernace = tolerance;
        }
    }

    if (wall_contact) {
        grab_checker_cast_climb(checker, player_collider, wall_contact, max_grab_height);
    } else if (ground_contact) {
        grab_checker_cast_hang(checker, player_collider, target_direction, ground_contact);
    } else {
        checker->cast_mode = GRAB_MODE_NONE;
        checker->position = gZeroVec;
    }

    return checker->grab_mode;
}

void grab_checker_clear(grab_checker_t* checker) {
    debugf("clear cast mode\n");
    checker->cast_mode = GRAB_MODE_NONE;
    checker->grab_mode = GRAB_MODE_NONE;
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