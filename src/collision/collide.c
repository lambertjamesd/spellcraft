#include "collide.h"

#include "epa.h"

#include "collision_scene.h"

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

        vector3AddScaled(object->position, &result.normal, -result.penetration, object->position);

        float velocityDot = vector3Dot(&object->velocity, &result.normal);

        struct Vector3 tangentVelocity;

        vector3AddScaled(&object->velocity, &result.normal, -velocityDot, &tangentVelocity);
        vector3Scale(&tangentVelocity, &tangentVelocity, 1.0f - object->type->friction);

        if (velocityDot < 0) {
            float bounce = -object->type->bounce;
            vector3AddScaled(&tangentVelocity, &result.normal, velocityDot * bounce, &object->velocity);
        }

        struct contact* contact = collision_scene_new_contact();

        if (!contact) {
            continue;
        }

        contact->normal = result.normal;
        contact->point = result.contactA;
        contact->other_object = 0;

        contact->next = object->active_contacts;
        object->active_contacts = contact;
    }
}