#include "./heal.h"

#include "../time/time.h"
#include "../entity/health.h"
#include "../objects/mana_gem.h"
#include "../render/render_scene.h"
#include "../collision/shapes/cylinder.h"
#include "assets.h"

#define BURST_HEAL          20.0f
#define BURST_EFFICIENCY    0.7f

#define HEAL_PER_SECOND     5.0f
#define EFFICIENCY          0.9f
#define MANA_PER_SECOND     (HEAL_PER_SECOND / EFFICIENCY)

#define AEO_SCALE           2.0f

static struct dynamic_object_type heal_collider = {
    CYLINDER_COLLIDER(AEO_SCALE, AEO_SCALE * 0.25f),
};

void spell_heal_render(void* data, struct render_batch* batch) {
    struct spell_heal* heal = (struct spell_heal*)data;

    if (!heal->flags.aoe) {
        return;
    }

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    struct TransformSingleAxis transform;
    transform.position = heal->data_source->position;
    transform.rotation = gRight2;
    transformSAToMatrix(&transform, mtx, AEO_SCALE);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(batch, spell_assets_get()->heal_aoe_mesh, mtxfp, 1, NULL, NULL);
}

void spell_heal_init(struct spell_heal* heal, struct spell_data_source* source, struct spell_event_options event_options) {
    heal->data_source = source;
    spell_data_source_retain(source);
    entity_id id = entity_id_new();

    if (event_options.modifiers.flaming) {
        heal->flags.instant = 1;
    } else {
        heal->flags.instant = 0;
    }

    if (event_options.modifiers.icy) {
        heal->flags.reverse = 1;
    } else {
        heal->flags.reverse = 0;
    }

    if (event_options.modifiers.earthy) {
        heal->flags.aoe = 1;
        
        dynamic_object_init(
            id, 
            &heal->aoe_trigger, 
            &heal_collider, 
            COLLISION_LAYER_DAMAGE_ENEMY | COLLISION_LAYER_DAMAGE_PLAYER, 
            &source->position, 
            NULL
        );
        heal->aoe_trigger.trigger_type = TRIGGER_TYPE_BASIC;
        collision_scene_add(&heal->aoe_trigger);
    } else {
        heal->flags.aoe = 0;
    }

    heal->mana_stored = 0.0f;

    render_scene_add(&source->position, 2.0f, spell_heal_render, heal);
}

void spell_heal_destroy(struct spell_heal* heal) {
    spell_data_source_release(heal->data_source);
    render_scene_remove(heal);

    if (heal->flags.aoe) {
        collision_scene_remove(&heal->aoe_trigger);
    }
}

void spell_heal_finish(struct spell_heal* heal) {
    if (heal->flags.reverse && heal->mana_stored > 0.0f) {
        mana_gem_new(&heal->data_source->position, heal->mana_stored);
    }
}

void spell_heal_apply_to_target(struct spell_heal* heal, struct spell_sources* spell_sources, entity_id id) {
    struct health* target = health_get(id);

    if (!target) {
        return;
    }

    float heal_amount;
    float mana_amount;
    float efficiency;

    if (heal->flags.instant) {
        heal_amount = BURST_HEAL;
        mana_amount = BURST_HEAL / BURST_EFFICIENCY;
        efficiency = BURST_EFFICIENCY;
    } else {
        heal_amount = HEAL_PER_SECOND * fixed_time_step;
        mana_amount = MANA_PER_SECOND * fixed_time_step;
        efficiency = EFFICIENCY;
    }

    if (!heal->flags.reverse) {
        if (target->current_health < target->max_health) {
            heal_amount = mana_pool_request(&spell_sources->mana_pool, mana_amount) * EFFICIENCY;
            health_heal(target, heal_amount);
        }
    } else {
        struct damage_info damage = {
            .amount = heal_amount,
            .direction = gZeroVec,
            .source = heal->data_source->target,
            .type = DAMAGE_TYPE_STEAL,
        };
        heal->mana_stored += health_damage(target, &damage) / efficiency;
    }
}

bool spell_heal_update(struct spell_heal* heal, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (heal->flags.aoe) {
        struct contact* contact = heal->aoe_trigger.active_contacts;

        while (contact) {
            spell_heal_apply_to_target(heal, spell_sources, contact->other_object);
            contact = contact->next;
        }
    } else {
        spell_heal_apply_to_target(heal, spell_sources, heal->data_source->target);
    }

    if (heal->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE || heal->flags.instant) {
        spell_heal_finish(heal);
        return false;
    }

    return true;
}