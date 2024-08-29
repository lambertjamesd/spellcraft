#include "cutscene_actor.h"

#include "../time/time.h"

bool cutscene_actor_update(struct cutscene_actor* actor) {
    if (actor->state == ACTOR_STATE_INACTIVE) {
        return false;
    }

    struct Vector3* pos = transform_mixed_get_position(&actor->transform);

    if (actor->state == ACTOR_STATE_MOVING) {
        if (vector3MoveTowards(
            pos, 
            actor->target,
            actor->speed * fixed_time_step,
            pos
        )) {
            actor->target = NULL;
            actor->state = ACTOR_STATE_FINISHED;
        }
    }

    if (actor->state == ACTOR_STATE_LOOKING) {
        struct Vector3 offset;
        vector3Sub(actor->target, pos, &offset);

        if (transform_rotate_towards(&actor->transform, &offset, actor->speed * fixed_time_step)) {
            actor->target = NULL;
            actor->state = ACTOR_STATE_FINISHED;
        }
    }

    return true;
}