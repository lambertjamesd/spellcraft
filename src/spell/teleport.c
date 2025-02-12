#include "./teleport.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"

#define TELEPORT_SPEED   (3.0f / DEFAULT_TIME_STEP)

void teleport_init(struct teleport* teleport, struct spell_data_source* source, struct spell_event_options event_options, enum teleport_dir dir) {
    teleport->target = source->target;
    teleport->saved_velocity = source->direction;
    teleport->teleport_time = 0.0f;
    teleport->dir = dir;
}

void teleport_destroy(struct teleport* teleport) {

}

bool teleport_update(struct teleport* teleport, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    struct dynamic_object* target = collision_scene_find_object(teleport->target);

    if (!target) {
        return false;
    }

    if (teleport->teleport_time) {
        teleport->teleport_time -= fixed_time_step;
        if (teleport->teleport_time <= 0.0f) {
            target->velocity = teleport->saved_velocity;
            return false;
        }
        return true;
    }


    struct Vector3 prev_vel = target->velocity;

    if (teleport->dir == TELEPORT_DIR_SIDE) {
        teleport->teleport_time = DEFAULT_TIME_STEP;
        vector3Scale(&teleport->saved_velocity, &target->velocity, TELEPORT_SPEED);
        target->position->y += 0.1f;
    } else if (dynamic_object_is_grounded(target)) {
        teleport->teleport_time = DEFAULT_TIME_STEP;
        vector3Scale(&gUp, &target->velocity, TELEPORT_SPEED);
        target->position->y += 0.1f;
    } else {
        teleport->teleport_time = DEFAULT_TIME_STEP * 10;
        vector3Scale(&gUp, &target->velocity, -TELEPORT_SPEED);
    }

    teleport->saved_velocity = prev_vel;

    return true;
}
