#include "collide_swept.h"

#include <math.h>
#include "collision_scene.h"
#include "collide.h"
#include "gjk.h"
#include "epa.h"
#include <stdio.h>

struct swept_dynamic_object {
    struct dynamic_object* object;
    struct Vector3 offset;
};

void swept_dynamic_object_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    struct swept_dynamic_object* obj = (struct swept_dynamic_object*)data;

    dynamic_object_minkowski_sum(obj->object, direction, output);

    if (vector3Dot(&obj->offset, direction) > 0.0f) {
        vector3Add(output, &obj->offset, output);
    }
}

void object_mesh_collide_data_init(
    struct object_mesh_collide_data* data,
    struct Vector3* prev_pos,
    struct mesh_collider* mesh,
    struct dynamic_object* object
) {
    data->prev_pos = prev_pos;
    data->mesh = mesh;
    data->object = object;
}

bool collide_object_swept_to_triangle(struct mesh_index* index, void* data, int triangle_index, int collision_layers) {
    struct object_mesh_collide_data* collide_data = (struct object_mesh_collide_data*)data;

    struct mesh_triangle triangle;
    triangle.vertices = collide_data->mesh->vertices;
    triangle.triangle = collide_data->mesh->triangles[triangle_index];

    if (!(surface_type_collision_layers[triangle.triangle.surface_type] & collision_layers)) {
        return false;
    }

    struct swept_dynamic_object swept;
    swept.object = collide_data->object;
    vector3Sub(collide_data->prev_pos, collide_data->object->position, &swept.offset);

    struct Simplex simplex;
    if (!gjkCheckForOverlap(&simplex, &triangle, mesh_triangle_minkowski_sum, &swept, swept_dynamic_object_minkowski_sum, &gRight)) {
        return false;
    }

    struct EpaResult result;

    if (epaSolveSwept(
        &simplex, 
        &triangle, 
        mesh_triangle_minkowski_sum, 
        &swept, 
        swept_dynamic_object_minkowski_sum, 
        collide_data->prev_pos,
        collide_data->object->position,
        &result
    )) {
        collide_data->hit_result = result;
        collide_data->surface_type = triangle.triangle.surface_type;
        return true;
    }

    struct Vector3 final_pos = *collide_data->object->position;
    *collide_data->object->position = *collide_data->prev_pos;

    if (epaSolve(
        &simplex,
        &triangle,
        mesh_triangle_minkowski_sum,
        collide_data->object,
        dynamic_object_minkowski_sum,
        &result
    )) {
        collide_data->hit_result = result;
        collide_data->surface_type = triangle.triangle.surface_type;
        return true;
    }

    *collide_data->object->position = final_pos;

    return false;
}

#define FACE_MARGIN 0.001f

void collide_object_swept_bounce(
    struct dynamic_object* object, 
    struct object_mesh_collide_data* collide_data,
    struct Vector3* start_pos
) {
    // this is the new prev position when iterating
    // over mulitple swept collisions
    *collide_data->prev_pos = *object->position;

    struct Vector3 move_amount;
    vector3Sub(start_pos, object->position, &move_amount);

    struct Vector3 move_amount_normal;
    vector3Project(&move_amount, &collide_data->hit_result.normal, &move_amount_normal);
    struct Vector3 move_amount_tangent;
    vector3Sub(&move_amount, &move_amount_normal, &move_amount_tangent);

    vector3Scale(&move_amount_normal, &move_amount_normal, -object->type->bounce);
    
    vector3Add(object->position, &move_amount_normal, object->position);
    vector3Add(object->position, &move_amount_tangent, object->position);
    vector3AddScaled(object->position, &collide_data->hit_result.normal, -collide_data->hit_result.penetration + FACE_MARGIN, object->position);

    // don't include friction on a bounce
    correct_velocity(&object->velocity, &collide_data->hit_result.normal, -1.0f, 0.0f, object->type->bounce);

    vector3Sub(object->position, start_pos, &move_amount);
    vector3Add(&move_amount, &object->bounding_box.min, &object->bounding_box.min);
    vector3Add(&move_amount, &object->bounding_box.max, &object->bounding_box.max);

    collide_add_contact(object, &collide_data->hit_result, collide_data->surface_type);
}

bool collide_object_to_mesh_swept(struct dynamic_object* object, struct mesh_collider* mesh, struct Vector3* prev_pos) {
    if (object->trigger_type != TRIGGER_TYPE_NONE) {
        return false;
    }

    struct object_mesh_collide_data collide_data;
    object_mesh_collide_data_init(&collide_data, prev_pos, mesh, object);

    struct Vector3 start_pos = *object->position;
    struct Vector3 offset;

    vector3Sub(
        object->position, 
        prev_pos, 
        &offset
    );

    if (!mesh_index_swept_lookup(
        &mesh->index, 
        &object->bounding_box, 
        &offset, 
        collide_object_swept_to_triangle, 
        &collide_data,
        object->collision_layers
    )) {
        return false;
    }

    collide_object_swept_bounce(object, &collide_data, &start_pos);

    return true;
}

bool collide_object_to_multiple_mesh_swept(struct dynamic_object* object, struct mesh_collider** meshes, int mesh_count, struct Vector3* prev_pos) {
    if (object->trigger_type != TRIGGER_TYPE_NONE) {
        return false;
    }

    struct object_mesh_collide_data collide_data;
    object_mesh_collide_data_init(&collide_data, prev_pos, NULL, object);

    struct Vector3 start_pos = *object->position;
    struct Vector3 offset;

    vector3Sub(
        object->position, 
        prev_pos, 
        &offset
    );

    bool did_hit = false;

    for (int i = 0; i < mesh_count; i += 1) {
        collide_data.mesh = meshes[i];
        if (mesh_index_swept_lookup(
            &meshes[i]->index, 
            &object->bounding_box, 
            &offset, 
            collide_object_swept_to_triangle, 
            &collide_data,
            object->collision_layers
        )) {
            did_hit = true;
        }
    }

    if (!did_hit) {
        return false;
    }

    collide_object_swept_bounce(object, &collide_data, &start_pos);

    return true;
}
