#include "collision_scene.h"

#include <malloc.h>
#include <stdbool.h>

#include "mesh_collider.h"
#include "collide.h"
#include "contact.h"

#define MIN_DYNAMIC_OBJECTS 64
#define MAX_ACTIVE_CONTACTS 128

struct collision_scene_element {
    struct dynamic_object* object;
};

struct collision_scene {
    struct collision_scene_element* elements;
    struct contact* next_free_contact;
    struct contact* all_contacts;
    uint16_t count;
    uint16_t capacity;

    struct mesh_collider* mesh_collider;
};

static struct collision_scene g_scene;

void collision_scene_reset() {
    free(g_scene.elements);

    g_scene.elements = malloc(sizeof(struct collision_scene_element) * MIN_DYNAMIC_OBJECTS);
    g_scene.capacity = MIN_DYNAMIC_OBJECTS;
    g_scene.count = 0;
    g_scene.all_contacts = malloc(sizeof(struct contact) * MAX_ACTIVE_CONTACTS);
    g_scene.next_free_contact = &g_scene.all_contacts[0];

    for (int i = 0; i + 1 < MAX_ACTIVE_CONTACTS; ++i) {
        g_scene.all_contacts[i].next = &g_scene.all_contacts[i + 1];
    }

    g_scene.all_contacts[MAX_ACTIVE_CONTACTS - 1].next = NULL;
}

void collision_scene_add(struct dynamic_object* object) {
    if (g_scene.count >= g_scene.capacity) {
        g_scene.capacity *= 2;
        g_scene.elements = realloc(g_scene.elements, sizeof(struct collision_scene_element) * g_scene.capacity);
    }

    struct collision_scene_element* next = &g_scene.elements[g_scene.count];

    next->object = object;

    g_scene.count += 1;
}

void collision_scene_remove(struct dynamic_object* object) {
    bool has_found = false;

    for (int i = 0; i < g_scene.count; ++i) {
        if (object == g_scene.elements[i].object) {
            has_found = true;
        }

        if (has_found) {
            g_scene.elements[i] = g_scene.elements[i + 1];
        }
    }

    if (has_found) {
        g_scene.count -= 1;
    }
}

void collision_scene_use_static_collision(struct mesh_collider* collider) {
    g_scene.mesh_collider = collider;
}

void collision_scene_remove_static_collision(struct mesh_collider* collider) {
    if (collider == g_scene.mesh_collider) {
        g_scene.mesh_collider = NULL;
    }
}

void collision_scene_collide() {
    struct Vector3 prev_pos[g_scene.count];

    for (int i = 0; i < g_scene.count; ++i) {
        struct collision_scene_element* element = &g_scene.elements[i];
        prev_pos[i] = *element->object->position;

        struct contact* last_contact = element->object->active_contacts;

        while (last_contact && last_contact->next) {
            last_contact = last_contact->next;
        }

        if (last_contact) {
            last_contact->next = g_scene.next_free_contact;
            g_scene.next_free_contact = element->object->active_contacts;
            element->object->active_contacts = NULL;
        }

        dynamic_object_update(element->object);

        if (g_scene.mesh_collider) {
            collide_object_to_mesh(element->object, g_scene.mesh_collider);
        }
    }
}

struct contact* collision_scene_new_contact() {
    if (!g_scene.next_free_contact) {
        return NULL;
    }

    struct contact* result = g_scene.next_free_contact;
    g_scene.next_free_contact = result->next;
    return result;
}