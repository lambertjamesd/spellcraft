#include "stasis.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"

#define MANA_PER_SECOND 2.0f

void stasis_init(struct stasis* stasis, struct spell_data_source* source, struct spell_event_options event_options) {
    stasis->data_source = spell_data_source_retain(source);
    mana_regulator_init(&stasis->mana_regulator, event_options.burst_mana, 8.0f);

    struct dynamic_object* target = collision_scene_find_object(stasis->data_source->target);

    if (target) {
        stasis->hover_location = *target->position;
        stasis->original_velocity = target->velocity;
    }
}

void stasis_destroy(struct stasis* stasis) {
    spell_data_source_release(stasis->data_source);

    struct dynamic_object* target = collision_scene_find_object(stasis->data_source->target);

    if (target) {
        target->velocity = stasis->original_velocity;
    }
}

bool stasis_update(struct stasis* stasis, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    float mana_got = mana_regulator_request(&stasis->mana_regulator, &spell_sources->mana_pool, MANA_PER_SECOND * fixed_time_step);

    if (!mana_got) {
        return false;
    }

    struct dynamic_object* target = collision_scene_find_object(stasis->data_source->target);

    if (!target) {
        return false;
    }

    *target->position = stasis->hover_location;
    target->velocity = gZeroVec;

    return stasis->data_source->flags.cast_state == SPELL_CAST_STATE_ACTIVE;
}
