#ifndef __PLAYER_GRAB_CHECKER_H__
#define __PLAYER_GRAB_CHECKER_H__

#include <stdbool.h>
#include "../math/vector3.h"
#include "../collision/dynamic_object.h"

enum grab_mode {
    GRAB_MODE_NONE,
    GRAB_MODE_CLIMB,
    GRAB_MODE_HANG,
};

typedef enum grab_mode grab_mode_t;

struct grab_checker {
    struct Vector3 position;
    dynamic_object_t collider;
    struct Vector3 climb_to;
    struct Vector3 target_pos;
    vector2_t target_rot;
    uint8_t grab_mode;
    uint8_t cast_mode;
};

typedef struct grab_checker grab_checker_t;

void grab_checker_init(grab_checker_t* checker, struct dynamic_object_type* collider_type);
grab_mode_t grab_checker_update(grab_checker_t* checker, dynamic_object_t* player_collider, struct Vector3* target_direction, float max_grab_height);

void grab_checker_clear(grab_checker_t* checker);

void grab_checker_destroy(grab_checker_t* checker);

void grab_checker_get_climb_to(grab_checker_t* checker, struct Vector3* out);

#endif