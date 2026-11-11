#include "burst_attack.h"

#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "../resource/animation_cache.h"
#include "../collision/shapes/cylinder.h"
#include "../time/time.h"
#include "../entity/health.h"

burst_attack_definition_t burst_attack_ground_fire = {
    .mesh_name = "rom:/meshes/spell/groundfire.tmesh",
    .animation_file = "rom:/meshes/spell/groundfire.anim",
    .animation = "attack",
    .player_anim = SPELL_ANIMATION_CAST_GROUND,

    .collider = {
        CYLINDER_COLLIDER(1.5f, 1.25f),
        .center = {0.0f, 1.25f, 0.0f},
    },

    .damage_source = {
        .amount = 20.0f,
        .type = DAMAGE_TYPE_FIRE,
    },

    .mana_cost = 5.0f,
};

void burst_attack_create_spell(burst_attack_t* burst_attack, float scale) {
    if (burst_attack->has_spell) {
        return;
    }

    vector2_t rotation;
    vector2LookDir(&rotation, &burst_attack->data_source->direction);
    transformSaInit(&burst_attack->transform, &burst_attack->data_source->position, &rotation, scale);

    burst_attack->has_spell = true;
    burst_attack->damage_source.amount *= scale;
    
    renderable_single_axis_init(&burst_attack->renderable, &burst_attack->transform, burst_attack->def->mesh_name);
    render_scene_add_renderable(&burst_attack->renderable, 0.0f);
    renderable_set_animator(&burst_attack->renderable, &burst_attack->animator);

    burst_attack->animations = animation_cache_load(burst_attack->def->animation_file);

    animator_init(&burst_attack->animator, burst_attack->renderable.mesh_render.armature.definition);
    animator_run_clip(&burst_attack->animator, animation_set_find_clip(burst_attack->animations, burst_attack->def->animation), 0.0f, false);

    dynamic_object_init(
        entity_id_new(), 
        &burst_attack->collider, 
        &burst_attack->def->collider,
        COLLISION_LAYER_DAMAGE_ENEMY, 
        &burst_attack->transform.position, 
        &burst_attack->transform.rotation
    );
    burst_attack->collider.scale = scale;
    dynamic_object_wake(&burst_attack->collider);
    dynamic_object_recalc_bb(&burst_attack->collider);

    burst_attack->collider.is_fixed = true;
    burst_attack->collider.weight_class = WEIGHT_CLASS_GHOST;
    burst_attack->collider.trigger_type = TRIGGER_TYPE_OVERLAP;

    collision_scene_add(&burst_attack->collider);
}

void burst_attack_destroy_spell(burst_attack_t* burst_attack) {
    if (!burst_attack->has_spell) {
        return;
    }
    burst_attack->has_spell = false;
    render_scene_remove(&burst_attack->renderable);
    renderable_destroy(&burst_attack->renderable);

    animation_cache_release(burst_attack->animations);

    collision_scene_remove(&burst_attack->collider);
}

void burst_attack_init(burst_attack_t* burst_attack, spell_data_source_t* source, burst_attack_definition_t* effect_definition) {
    vector2_t rotation;
    vector2LookDir(&rotation, &source->direction);
    transformSaInit(&burst_attack->transform, &source->position, &rotation, 1.0f);

    burst_attack->def = effect_definition;
    burst_attack->data_source = spell_data_source_retain(source);
    burst_attack->has_spell = false;
    burst_attack->spell_phase = SPELL_BURST_PHASE_START;
    
    damaged_set_reset(&burst_attack->damaged_set);

    spell_data_source_set_cast_source(source, SPELL_CAST_SOURCE_FEET);

    spell_data_source_request_animation(source, burst_attack->def->player_anim);

    burst_attack->damage_source = effect_definition->damage_source;
}

void burst_attack_destroy(burst_attack_t* burst_attack) {
    burst_attack_destroy_spell(burst_attack);
    spell_data_source_release(burst_attack->data_source);
}

bool burst_attack_update(burst_attack_t* burst_attack, spell_sources_t* spell_sources) {
    animator_update(&burst_attack->animator, fixed_time_step);

    if (burst_attack->has_spell) {
        switch (burst_attack->spell_phase) {
            case SPELL_BURST_PHASE_START:
                if (burst_attack->animator.events.attack) {
                    burst_attack->spell_phase = SPELL_BURST_PHASE_DAMAGE;
                }
                break;
            case SPELL_BURST_PHASE_DAMAGE:
                if (!burst_attack->animator.events.attack) {
                    burst_attack->spell_phase = SPELL_BURST_PHASE_END;
                } else {
                    health_apply_contact_damage(
                        burst_attack->collider.active_contacts, 
                        &burst_attack->damage_source, 
                        &burst_attack->damaged_set
                    );
                }
                break;
            case SPELL_BURST_PHASE_END:
                if (burst_attack->has_spell && !animator_is_running(&burst_attack->animator)) {
                    burst_attack_destroy_spell(burst_attack);
                }
                break;
        }
    } else if (burst_attack->spell_phase == SPELL_BURST_PHASE_START) {
        if (burst_attack->data_source->flags.cast_state && !burst_attack->has_spell) {
            float mana = mana_pool_request(&spell_sources->mana_pool, burst_attack->def->mana_cost);
            if (mana > 0.0f) {
                burst_attack_create_spell(burst_attack, mana / burst_attack->def->mana_cost);
            } else {
                burst_attack->spell_phase = SPELL_BURST_PHASE_END;
            }
            
        }
    }

    return burst_attack->has_spell || burst_attack->spell_phase != SPELL_BURST_PHASE_END;
}