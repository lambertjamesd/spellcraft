#ifndef __ENTITY_INTERACTABLE_H__
#define __ENTITY_INTERACTABLE_H__

#include "entity_id.h"
#include <stdint.h>

struct interactable;

typedef void (*interaction_callback)(struct interactable* interactable, entity_id from);

struct interactable_flags {
    uint16_t grabbable: 1;
};

struct interactable {
    entity_id id;
    struct interactable_flags flags;
    interaction_callback callback;
    void* data;
};

typedef struct interactable interactable_t;

void interactable_reset();

void interactable_init(struct interactable* interactable, entity_id id, interaction_callback callback, void* data);
void interactable_destroy(struct interactable* interactable);

struct interactable* interactable_get(entity_id id);

#endif