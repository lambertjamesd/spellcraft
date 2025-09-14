#include "ground_movement.h"

#include "../time/time.h"
#include "../collision/collision_scene.h"

bool move_over_ground(struct Vector3* position, struct Vector3* direction, move_over_ground_def_t* definition, float max_downward_movement) {
    struct Vector3 cast_from = *direction;
    vector3Scale(&cast_from, &cast_from, definition->move_speed * fixed_time_step);
    vector3Add(&cast_from, position, &cast_from);

    cast_from.y += definition->max_upward_movment;

    struct mesh_shadow_cast_result cast_result;

    if (!collision_scene_shadow_cast(&cast_from, &cast_result)) {
        if (definition->can_fly) {
            position->x = cast_from.x;
            position->z = cast_from.z;
        }

        return false;
    }

    if (cast_from.y - cast_result.y > max_downward_movement + definition->max_upward_movment + definition->height_offset) {
        if (definition->can_fly) {
            position->x = cast_from.x;
            position->z = cast_from.z;
        }

        return false;
    }

    position->x = cast_from.x;
    position->y = cast_result.y + definition->height_offset;
    position->z = cast_from.z;

    return true;
}