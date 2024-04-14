#include "interactable.h"

#include "../util/hash_map.h"

static struct hash_map interactable_entity_mapping;

void interactable_reset() {
    hash_map_destroy(&interactable_entity_mapping);
    hash_map_init(&interactable_entity_mapping, 32);
}

void interactable_init(struct interactable* interactable, entity_id id, interaction_callback callback, void* data) {
    interactable->id = id;
    interactable->callback = callback;
    interactable->data = data;
    hash_map_set(&interactable_entity_mapping, id, interactable);
}

void interactable_destroy(struct interactable* interactable) {
    hash_map_delete(&interactable_entity_mapping, interactable->id);
}

struct interactable* interactable_get(entity_id id) {
    return hash_map_get(&interactable_entity_mapping, id);
}