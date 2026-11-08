#ifndef __SPELL_BURST_ATTACK_H__
#define __SPELL_BURST_ATTACK_H__

#include <stdbool.h>
#include "spell_sources.h"
#include "spell_event.h"
#include "../collision/collider_shape.h"
#include "../collision/dynamic_object.h"
#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../render/animator.h"
#include "../render/animation_clip.h"

struct burst_attack_definition {
    const char* mesh_name;
    const char* animation_file;
    const char* animation;
    collider_shape_t collider;
};

typedef struct burst_attack_definition burst_attack_definition_t;

struct burst_attack {
    transform_sa_t transform;
    renderable_t renderable;
    animator_t animator;
    animation_set_t* animations;
    dynamic_object_t collider;
    struct spell_data_source* data_source;
};

typedef struct burst_attack burst_attack_t;

void burst_attack_init(burst_attack_t* burst_attack, struct spell_data_source* source, struct spell_event_options event_options, burst_attack_definition_t* effect_definition);
void burst_attack_destroy(burst_attack_t* burst_attack);

bool burst_attack_update(burst_attack_t* burst_attack);

#endif