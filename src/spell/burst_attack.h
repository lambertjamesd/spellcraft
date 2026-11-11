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
#include "../entity/damage.h"

struct burst_attack_definition {
    const char* mesh_name;
    const char* animation_file;
    const char* animation;
    dynamic_object_type_t collider;
    spell_animation_t player_anim;
    damage_source_t damage_source;
    float mana_cost;
    float size_scale;
};

typedef struct burst_attack_definition burst_attack_definition_t;

enum spell_burst_phase {
    SPELL_BURST_PHASE_START,
    SPELL_BURST_PHASE_DAMAGE,
    SPELL_BURST_PHASE_END,
};

struct burst_attack {
    transform_sa_t transform;
    renderable_t renderable;
    animator_t animator;
    animation_set_t* animations;
    dynamic_object_t collider;
    spell_data_source_t* data_source;
    burst_attack_definition_t* def;
    bool has_spell;
    uint8_t spell_phase;
    struct damaged_set damaged_set;
    damage_source_t damage_source;
};

typedef struct burst_attack burst_attack_t;

extern burst_attack_definition_t burst_attack_ground_fire;

void burst_attack_init(burst_attack_t* burst_attack, spell_data_source_t* source, burst_attack_definition_t* effect_definition);
void burst_attack_destroy(burst_attack_t* burst_attack);

bool burst_attack_update(burst_attack_t* burst_attack, spell_sources_t* spell_sources);

#endif