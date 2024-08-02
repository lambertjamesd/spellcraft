#include "collide_swept.h"

#include "collision_scene.h"
#include "collide.h"
#include "gjk.h"
#include "epa.h"

struct swept_dynamic_object {
    struct dynamic_object* object;
    struct Vector3 offset;
};

void swept_dynamic_object_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    struct swept_dynamic_object* obj = (struct swept_dynamic_object*)data;

    dynamic_object_minkowski_sum(obj->object, direction, output);

    if (vector3Dot(&obj->offset, direction)) {
        vector3Add(output, &obj->offset, output);
    }
}

struct object_mesh_collide_data {
    struct mesh_collider* mesh;
    struct dynamic_object* object;
    struct mesh_triangle triangle;
};

bool collide_object_swept_to_triangle(struct mesh_index* index, void* data, int triangle_index) {
    triangle.triangle = mesh->triangles[indices[i]];

    struct Simplex simplex;
    if (!gjkCheckForOverlap(&simplex, &triangle, mesh_triangle_minkowski_sum, &swept, swept_dynamic_object_minkowski_sum, &gRight)) {
        continue;
    }

    struct EpaResult result;

    struct Vector3 curr_pos = *object->position;

    if (epaSolveSwept(
        &simplex, 
        &triangle, 
        mesh_triangle_minkowski_sum, 
        &swept, 
        swept_dynamic_object_minkowski_sum, 
        prev_pos,
        object->position,
        &result
    )) {
        collide_add_contact(object, &result);
    } else if (epaSolve(
        &simplex, 
        &triangle, 
        mesh_triangle_minkowski_sum, 
        object, 
        dynamic_object_minkowski_sum, 
        &result
    )) {
        correct_overlap(object, &result, -1.0f, object->type->friction, object->type->bounce);
        collide_add_contact(object, &result);
    } else {
        continue;
    }

    struct Vector3 move_amount;
    vector3Sub(object->position, &curr_pos, &move_amount);
    vector3Add(&move_amount, &object->bounding_box.min, &object->bounding_box.min);
    vector3Add(&move_amount, &object->bounding_box.max, &object->bounding_box.max);

    vector3Sub(prev_pos, object->position, &swept.offset);
}

bool collide_object_to_mesh_swept(struct dynamic_object* object, struct mesh_collider* mesh, struct Vector3* prev_pos, struct swept_collide_result* result) {
    if (object->is_trigger) {
        return false;
    }

    struct swept_dynamic_object swept;
    swept.object = object;
    vector3Sub(prev_pos, object->position, &swept.offset);

    struct Box3D sweptBox;
    box3DExtendDirection(&object->bounding_box, &swept.offset, &sweptBox);

    struct object_mesh_collide_data collide_data;

    mesh_index_swept_lookup(&mesh->index, &object->bounding_box, &swept.offset, collide_object_swept_to_triangle, &collide_data);
}