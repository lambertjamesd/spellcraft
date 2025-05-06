#include "./heal.h"

#include "../time/time.h"
#include "../entity/health.h"
#include "../objects/mana_gem.h"

#define BURST_HEAL          20.0f
#define BURST_EFFICIENCY    0.7f

#define HEAL_PER_SECOND     5.0f
#define EFFICIENCY          0.9f
#define MANA_PER_SECOND     (HEAL_PER_SECOND / EFFICIENCY)

void spell_heal_init(struct spell_heal* heal, struct spell_data_source* source, struct spell_event_options event_options) {
    heal->data_source = source;
    spell_data_source_retain(source);

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
    } else {
        heal->flags.aoe = 0;
    }

    heal->mana_stored = 0.0f;
}

void spell_heal_destroy(struct spell_heal* heal) {
    spell_data_source_release(heal->data_source);
}

void spell_heal_finish(struct spell_heal* heal) {
    if (heal->flags.reverse && heal->mana_stored > 0.0f) {
        mana_gem_new(&heal->data_source->position, heal->mana_stored);
    }
}

bool spell_heal_update(struct spell_heal* heal, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    struct health* target = health_get(heal->data_source->target);

    if (!target) {
        spell_heal_finish(heal);
        return false;
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
        health_damage(target, &damage);
        heal->mana_stored += mana_amount;
    }

    if (heal->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE || heal->flags.instant) {
        spell_heal_finish(heal);
        return false;
    }

    return true;
}