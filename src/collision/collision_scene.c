#include "collision_scene.h"

#include <malloc.h>
#include <stdbool.h>

#include "mesh_collider.h"
#include "collide.h"

#define MIN_DYNAMIC_OBJECTS 64

struct collision_scene_element {
    struct dynamic_object* object;
};

struct collision_scene {
    struct collision_scene_element* elements;
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

void collision_scene_collide() {
    struct Vector3 prev_pos[g_scene.count];

    for (int i = 0; i < g_scene.count; ++i) {
        prev_pos[i] = *g_scene.elements[i].object->position;

        dynamic_object_update(g_scene.elements[i].object);

        if (g_scene.mesh_collider) {
            collide_object_to_mesh(g_scene.elements[i].object, g_scene.mesh_collider);
        }
    }
}