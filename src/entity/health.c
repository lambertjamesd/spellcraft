#include "health.h"

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
}

void health_set_callback(struct health* health, health_damage_callback callback, void* data) {
    health->callback = callback;
    health->callback_data = data;
}

void health_destroy(struct health* health) {
    hash_map_delete(&health_entity_mapping, health->entity_id);
    update_remove(health);
}

void health_damage(struct health* health, struct damage_info* damage) {
    if (health->max_health) {
        health->current_health -= damage->amount;
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

    if (health->callback) {
        health->callback(health->callback_data, damage);
    }
}

void health_damage_id(entity_id target, struct damage_info* damage) {
    struct health* health = health_get(target);

    if (!health) {
        return;
    }

    health_damage(health, damage);
}

void health_apply_contact_damage(struct dynamic_object* damage_source, float amount, enum damage_type type) {
    struct contact* curr = damage_source->active_contacts;

    struct damage_info damage;
    damage.amount = amount;
    damage.type = type;

    while (curr) {
        struct health* target_health = health_get(curr->other_object);

        if (!target_health) {
            curr = curr->next;
            continue;
        }

        damage.source = curr->other_object;
        damage.direction = curr->normal;

        health_damage(target_health, &damage);
        curr = curr->next;
    }
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

static enum damage_type element_to_damage_type[] = {
    [ELEMENT_TYPE_FIRE] = DAMAGE_TYPE_FIRE,
    [ELEMENT_TYPE_ICE] = DAMAGE_TYPE_ICE,
    [ELEMENT_TYPE_LIGHTNING] = DAMAGE_TYPE_LIGHTING,
    [ELEMENT_TYPE_WATER] = DAMAGE_TYPE_WATER,
};

enum damage_type health_determine_damage_type(enum element_type element_type) {
    return element_to_damage_type[element_type];
}