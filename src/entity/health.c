#include "health.h"

#include "../math/mathf.h"
#include "../util/hash_map.h"
#include "../time/time.h"
#include "../collision/dynamic_object.h"
#include <stddef.h>

static struct hash_map health_entity_mapping;

void health_reset() {
    hash_map_destroy(&health_entity_mapping);
    hash_map_init(&health_entity_mapping, 32);
}

void health_update(void *data) {
    struct health* health = (struct health*)data;

    if (health->status_timer) {
        health->status_timer -= fixed_time_step;

        if (health->status_timer < 0.0f) {
            health->status_timer = 0.0f;
        }
    }
}

void health_init(struct health* health, entity_id id, float max_health) {
    health->entity_id = id;
    health->max_health = max_health;
    health->current_health = max_health;
    health->status_timer = 0.0f;

    hash_map_set(&health_entity_mapping, id, health);

    update_add(health, health_update, 0, UPDATE_LAYER_WORLD);

    health->callback = NULL;
    health->callback_data = NULL;
    health->health_shield = NULL;
}

void health_set_callback(struct health* health, health_damage_callback callback, void* data) {
    health->callback = callback;
    health->callback_data = data;
}

void health_destroy(struct health* health) {
    hash_map_delete(&health_entity_mapping, health->entity_id);
    update_remove(health);
}

float health_damage(struct health* health, struct damage_info* damage) {
    if (health->health_shield && health_shield_does_block(health->health_shield, damage)) {
        return 0.0f;
    }

    float amount = damage->amount;

    if (health->callback) {
        amount = health->callback(health->callback_data, damage);

        if (amount <= 0.0f) {
            return 0.0f;
        }
    }

    float result = 0.0f;

    if (health->max_health) {
        if (health->current_health >= amount) {
            result = amount;
            health->current_health -= amount;
        } else {
            result = health->current_health;
            health->current_health = 0.0f;
        }
    }

    if (damage->type & DAMAGE_TYPE_FIRE) {
        health->status_timer = 7;
        health->current_status = DAMAGE_TYPE_FIRE;
    } else if (damage->type & DAMAGE_TYPE_ICE) {
        health->status_timer = 7;
        health->current_status = DAMAGE_TYPE_ICE;
    } else if (damage->type & DAMAGE_TYPE_LIGHTING) {
        health->status_timer = 3;
        health->current_status = DAMAGE_TYPE_LIGHTING;
    } else if (damage->type & DAMAGE_TYPE_WATER) {
        health->status_timer = 7;
        health->current_status = DAMAGE_TYPE_WATER;
    }

    return result;
}

float health_damage_id(entity_id target, struct damage_info* damage) {
    struct health* health = health_get(target);

    if (!health) {
        return 0.0f;
    }

    return health_damage(health, damage);
}

void health_heal(struct health* health, float amount) {
    health->current_health = minf(health->max_health, health->current_health + amount);
}

bool health_apply_contact_damage(struct dynamic_object* damage_source, float amount, enum damage_type type) {
    struct contact* curr = damage_source->active_contacts;

    struct damage_info damage;
    damage.amount = amount;
    damage.type = type;

    bool did_hit = false;

    while (curr) {
        struct health* target_health = health_get(curr->other_object);

        if (!target_health) {
            curr = curr->next;
            continue;
        }

        damage.source = curr->other_object;
        damage.direction = curr->normal;

        health_damage(target_health, &damage);
        did_hit = true;
        curr = curr->next;
    }

    return did_hit;
}

struct health* health_get(entity_id id) {
    return hash_map_get(&health_entity_mapping, id);
}

bool health_is_burning(struct health* health) {
    return health->status_timer > 0.0f && health->current_status == DAMAGE_TYPE_FIRE;
}

bool health_is_frozen(struct health* health) {
    return health->status_timer > 0.0f && health->current_status == DAMAGE_TYPE_ICE;
}

bool health_is_shocked(struct health* health) {
    return health->status_timer > 0.0f && health->current_status == DAMAGE_TYPE_LIGHTING;
}

bool health_is_alive(struct health* health) {
    return health->current_health > 0.0f;
}

void health_add_shield(struct health* health, struct health_shield* health_shield) {
    health->health_shield = health_shield;
}

void health_remove_shield(struct health* health, struct health_shield* health_shield) {
    if (health->health_shield == health_shield) {
        health->health_shield = NULL;
    }
}

static enum damage_type element_to_damage_type[] = {
    [ELEMENT_TYPE_FIRE] = DAMAGE_TYPE_FIRE,
    [ELEMENT_TYPE_ICE] = DAMAGE_TYPE_ICE,
    [ELEMENT_TYPE_LIGHTNING] = DAMAGE_TYPE_LIGHTING,
    [ELEMENT_TYPE_WATER] = DAMAGE_TYPE_WATER,
};

enum damage_type health_determine_damage_type(enum element_type element_type) {
    return element_to_damage_type[element_type];
}