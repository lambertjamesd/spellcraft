#include "collide.h"

#include "epa.h"

void collide_object_to_mesh(struct dynamic_object* object, struct mesh_collider* mesh) {
    struct mesh_triangle triangle;

    triangle.vertices = mesh->vertices;

    for (int i = 0; i < mesh->triangle_count; ++i) {
        triangle.triangle = mesh->triangles[i];

        struct Simplex simplex;
        if (!gjkCheckForOverlap(&simplex, &triangle, mesh_triangle_minkowski_sum, object, dynamic_object_minkowski_sum, &gRight)) {
            continue;
        }

        struct EpaResult result;

        epaSolve(&simplex, &triangle, mesh_triangle_minkowski_sum, object, dynamic_object_minkowski_sum, &result);

        vector3AddScaled(object->position, &result.normal, result.penetration, object->position);
    }
}