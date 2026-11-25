#include "spell_data_source.h"

#include <assert.h>

void spell_data_source_pool_init(struct spell_data_source_pool* pool) {
    for (int i = 0; i < MAX_SPELL_DATA_SOURCES; ++i) {
        pool->data_sources[i].reference_count = 0;
    }

    pool->next_data_source = 0;
}

struct spell_data_source* spell_data_source_pool_get(struct spell_data_source_pool* pool) {
    for (int i = 0; i < MAX_SPELL_DATA_SOURCES; ++i) {
        struct spell_data_source* current = &pool->data_sources[pool->next_data_source];

        pool->next_data_source += 1;

        if (pool->next_data_source == MAX_SPELL_DATA_SOURCES) {
            pool->next_data_source = 0;
        }

        if (current->reference_count == 0) {
            return current;
        }
    }

    return 0;
}

struct spell_data_source* spell_data_source_retain(struct spell_data_source* data_source) {
    if (!data_source) {
        return NULL;
    }

    data_source->reference_count += 1;
    return data_source;
}

void spell_data_source_release(struct spell_data_source* data_source) {
    if (!data_source) {
        return;
    }

    assert(data_source->reference_count > 0);

    data_source->reference_count -= 1;
}

bool spell_data_source_request_animation(struct spell_data_source* data_source, enum spell_animation animation, bool skip_blending) {
    if (!data_source->flags.has_animator) {
        return false;
    }

    data_source->request_animation = animation;
    data_source->flags.skip_blending = skip_blending;

    return true;
}

void spell_data_source_cancel_animation(struct spell_data_source* data_source) {
    data_source->flags.is_animating = false;
}

void spell_data_source_apply_transform_sa(struct spell_data_source* data_source, spell_cast_source_t source, struct TransformSingleAxis* transform) {
    transform->position = data_source->position;
    spell_data_source_get_pos(data_source, source, &transform->position);
    vector3_t dir;
    spell_data_source_get_dir(data_source, source, &dir);

    transform->rotation.x = dir.z;
    transform->rotation.y = -dir.x;

    vector2Normalize(&transform->rotation, &transform->rotation);

    transform->scale = 1.0f;
}

enum element_type spell_data_source_determine_element(rune_pattern_t rune) {
    if (rune.flaming) {
        if (rune.watery) {
            return ELEMENT_TYPE_LIGHTNING;
        }

        return ELEMENT_TYPE_FIRE;
    }

    if (rune.watery) {
        return ELEMENT_TYPE_WATER;
    }

    return ELEMENT_TYPE_NONE;
}