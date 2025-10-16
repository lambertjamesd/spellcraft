#ifndef __PLAYER_GRAB_CHECKER_H__
#define __PLAYER_GRAB_CHECKER_H__

#include <stdbool.h>
#include "../math/vector3.h"
#include "../collision/dynamic_object.h"

struct grab_checker {
    struct Vector3 position;
    dynamic_object_t collider;
    struct Vector3 climb_to;
    struct Vector2 target_pos;
    float last_player_y;
    bool can_grab;
    bool did_cast;
    uint16_t grab_timer;
};

typedef struct grab_checker grab_checker_t;

void grab_checker_init(grab_checker_t* checker, struct dynamic_object_type* collider_type);
bool grab_checker_update(grab_checker_t* checker, dynamic_object_t* player_collider, struct Vector3* target_direction);
void grab_checker_destroy(grab_checker_t* checker);

void grab_checker_get_climb_to(grab_checker_t* checker, struct Vector3* out);

#endif