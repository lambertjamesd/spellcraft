#include "jump.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../render/render_scene.h"
#include "../entity/health.h"
#include "../util/flags.h"
#include "assets.h"

#define INITIAL_IMPULSE 3.0f
#define HOVER_ACCEL     10.0f

void jump_init(struct jump* jump, struct spell_data_source* source, struct spell_event_options event_options) {
    jump->data_source = spell_data_source_retain(source);
    
    jump->did_start = false;

    struct dynamic_object* target = collision_scene_find_object(jump->data_source->target);

    if (target) {
        target->is_jumping += 1;
    }
}

void jump_destroy(struct jump* jump) {
    struct dynamic_object* target = collision_scene_find_object(jump->data_source->target);

    if (target) {
        target->is_jumping -= 1;
    }
}

bool jump_update(struct jump* jump, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    struct dynamic_object* target = collision_scene_find_object(jump->data_source->target);

    if (!target) {
        return false;
    }

    if (!jump->did_start) {
        jump->did_start = true;

        if (dynamic_object_is_grounded(target)) {
            target->velocity.y += INITIAL_IMPULSE;
        }
    } else {
        target->velocity.y += HOVER_ACCEL * fixed_time_step;
    }

    return jump->data_source->flags.cast_state == SPELL_CAST_STATE_ACTIVE;
}
