#include "collision_scene.h"

#include <malloc.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include "mesh_collider.h"
#include "collide.h"
#include "collide_swept.h"
#include "contact.h"
#include "../util/hash_map.h"

struct collision_scene g_scene;

void collision_scene_reset() {
    free(g_scene.elements);
    free(g_scene.all_contacts);
    hash_map_destroy(&g_scene.entity_mapping);

    hash_map_init(&g_scene.entity_mapping, MIN_DYNAMIC_OBJECTS);

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

    hash_map_set(&g_scene.entity_mapping, object->entity_id, object);
}


struct dynamic_object* collision_scene_find_object(entity_id id) {
    if (!id) {
        return 0;
    }

    return hash_map_get(&g_scene.entity_mapping, id);
}

void collision_scene_return_contacts(struct dynamic_object* object) {
    struct contact* last_contact = object->active_contacts;

    while (last_contact && last_contact->next) {
        last_contact = last_contact->next;
    }

    if (last_contact) {
        last_contact->next = g_scene.next_free_contact;
        g_scene.next_free_contact = object->active_contacts;
        object->active_contacts = NULL;
    }
}

void collision_scene_remove(struct dynamic_object* object) {
    bool has_found = false;

    for (int i = 0; i < g_scene.count; ++i) {
        if (object == g_scene.elements[i].object) {
            collision_scene_return_contacts(object);
            has_found = true;
        }

        if (has_found) {
            g_scene.elements[i] = g_scene.elements[i + 1];
        }
    }

    if (has_found) {
        g_scene.count -= 1;
    }

    hash_map_delete(&g_scene.entity_mapping, object->entity_id);
}

void collision_scene_use_static_collision(struct mesh_collider* collider) {
    g_scene.mesh_collider = collider;
}

void collision_scene_remove_static_collision(struct mesh_collider* collider) {
    if (collider == g_scene.mesh_collider) {
        g_scene.mesh_collider = NULL;
    }
}

struct collide_edge {
    uint16_t is_start_edge: 1;
    uint16_t object_index: 15;
    short x;
};

int collide_edge_compare(struct collide_edge a, struct collide_edge b) {
    if (a.x == b.x) {
        return b.is_start_edge - a.is_start_edge;
    }

    return a.x - b.x;
}

void collide_edge_sort(struct collide_edge* edges, struct collide_edge* tmp, int start, int end) {
    if (start + 1 >= end) {
        return;
    }

    int mid = (start + end) >> 1;

    collide_edge_sort(edges, tmp, start, mid);
    collide_edge_sort(edges, tmp, mid, end);

    int a = start;
    int b = mid;
    int output = start;

    while (a < mid || b < end) {
        if (b >= end || (a < mid && collide_edge_compare(edges[a], edges[b]) < 0)) {
            tmp[output] = edges[a];
            ++output;
            ++a;
        } else {
            tmp[output] = edges[b];
            ++output;
            ++b;
        }
    }

    for (int i = start; i < end; ++i) {
        edges[i] = tmp[i];
    }
}

void collision_scene_collide_dynamic() {
    int edge_count = g_scene.count * 2;

    struct collide_edge collide_edges[edge_count];

    struct collide_edge* curr_edge = &collide_edges[0];

    for (int i = 0; i < g_scene.count; ++i) {
        struct collision_scene_element* element = &g_scene.elements[i];

        curr_edge->is_start_edge = 1;
        curr_edge->object_index = i;
        curr_edge->x = (short)(element->object->bounding_box.min.x * 32.0f);

        curr_edge += 1;

        curr_edge->is_start_edge = 0;
        curr_edge->object_index = i;
        curr_edge->x = (short)(element->object->bounding_box.max.x * 32.0f);

        curr_edge += 1;
    }

    struct collide_edge tmp[edge_count];
    collide_edge_sort(collide_edges, tmp, 0, edge_count);

    uint16_t active_objects[g_scene.count];
    int active_object_count = 0;

    for (int edge_index = 0; edge_index < edge_count; edge_index += 1) {
        struct collide_edge edge = collide_edges[edge_index];

        if (edge.is_start_edge) {
            struct dynamic_object* a = g_scene.elements[edge.object_index].object;

            for (int active_index = 0; active_index < active_object_count; active_index += 1) {
                struct dynamic_object* b = g_scene.elements[active_objects[active_index]].object;

                if (box3DHasOverlap(&a->bounding_box, &b->bounding_box)) {
                    collide_object_to_object(a, b);
                }
            }

            active_objects[active_object_count] = edge.object_index;
            active_object_count += 1;
            
        } else {
            int found_index = -1;

            for (int active_index = 0; active_index < active_object_count; active_index += 1) {
                if (active_objects[active_index] == edge.object_index) {
                    found_index = active_index;
                    break;
                }
            }

            assert(found_index != -1);

            // remove item by replacing it with the last one
            active_objects[found_index] = active_objects[active_object_count - 1];
            active_object_count -= 1;
        }
    }
}

#define MAX_SWEPT_ITERATIONS    5

void collision_scene_collide_single(struct dynamic_object* object, struct Vector3* prev_pos) {
    for (int i = 0; i < MAX_SWEPT_ITERATIONS; i += 1) {
        struct Vector3 offset;
        vector3Sub(object->position, prev_pos, &offset);
        struct Vector3 bbSize;
        vector3Sub(&object->bounding_box.max, &object->bounding_box.min, &bbSize);
        vector3Scale(&bbSize, &bbSize, 0.5f);

        if (fabs(offset.x) > bbSize.x ||
            fabs(offset.y) > bbSize.y ||
            fabs(offset.z) > bbSize.z
        ) {
            if (!collide_object_to_mesh_swept(object, g_scene.mesh_collider, prev_pos)) {
                return;
            }
        } else {
            collide_object_to_mesh(object, g_scene.mesh_collider);
            return;
        }
    }

    // too many swept iterations
    // to prevent tunneling just move 
    // the object back to the previous known
    // valid location
    *object->position = *prev_pos;
}

void collision_scene_collide() {
    struct Vector3 prev_pos[g_scene.count];

    for (int i = 0; i < g_scene.count; ++i) {
        struct collision_scene_element* element = &g_scene.elements[i];
        prev_pos[i] = *element->object->position;

        collision_scene_return_contacts(element->object);

        dynamic_object_update(element->object);

        dynamic_object_recalc_bb(element->object);
    }

    for (int i = 0; i < g_scene.count; ++i) {
        struct collision_scene_element* element = &g_scene.elements[i];

        if (!g_scene.mesh_collider) {
            continue;
        }

        collision_scene_collide_single(element->object, &prev_pos[i]);

        element->object->is_out_of_bounds = mesh_index_is_contained(&g_scene.mesh_collider->index, element->object->position);
    }

    collision_scene_collide_dynamic();
}

struct contact* collision_scene_new_contact() {
    if (!g_scene.next_free_contact) {
        return NULL;
    }

    struct contact* result = g_scene.next_free_contact;
    g_scene.next_free_contact = result->next;
    return result;
}

struct positioned_shape {
    struct dynamic_object_type* type;
    struct Vector3* center;
};

void positioned_shape_mink_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    struct positioned_shape* shape = (struct positioned_shape*)data;
    shape->type->minkowsi_sum(&shape->type->data, direction, output);
    vector3Add(output, shape->center, output);
}

void collision_scene_query(struct dynamic_object_type* shape, struct Vector3* center, int collision_layers, collision_scene_query_callback callback, void* callback_data) {
    struct Box3D bounding_box;
    shape->bounding_box(&shape->data, NULL, &bounding_box);
    vector3Add(&bounding_box.min, center, &bounding_box.min);
    vector3Add(&bounding_box.max, center, &bounding_box.max);

    struct positioned_shape positioned_shape;

    positioned_shape.type = shape;
    positioned_shape.center = center;

    for (int i = 0; i < g_scene.count; ++i) {
        struct collision_scene_element* element = &g_scene.elements[i];

        if (!(element->object->collision_layers & collision_layers)) {
            continue;
        }

        if (!box3DHasOverlap(&bounding_box, &element->object->bounding_box)) {
            continue;
        }

        struct Simplex simplex;

        struct Vector3 first_dir;
        vector3Sub(center, element->object->position, &first_dir);

        if (!gjkCheckForOverlap(&simplex, &positioned_shape, positioned_shape_mink_sum, element->object, dynamic_object_minkowski_sum, &first_dir)) {
            continue;;
        }

        callback(callback_data, element->object);
    }
}