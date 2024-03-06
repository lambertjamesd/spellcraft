#include "collide.h"

#include "epa.h"

#include "collision_scene.h"
#include "../util/flags.h"

void correct_overlap(struct dynamic_object* object, struct EpaResult* result, float ratio, float friction, float bounce) {
    vector3AddScaled(object->position, &result->normal, result->penetration * ratio, object->position);

    float velocityDot = vector3Dot(&object->velocity, &result->normal);

    if ((velocityDot < 0) == (ratio < 0)) {
        struct Vector3 tangentVelocity;

        vector3AddScaled(&object->velocity, &result->normal, -velocityDot, &tangentVelocity);
        vector3Scale(&tangentVelocity, &tangentVelocity, 1.0f - friction);

        vector3AddScaled(&tangentVelocity, &result->normal, velocityDot * -bounce, &object->velocity);
    }
}

void collide_object_to_mesh(struct dynamic_object* object, struct mesh_collider* mesh) {   
    if (object->is_trigger) {
        return;
    }

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

        correct_overlap(object, &result, -1.0f, object->type->friction, object->type->bounce);

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

void collide_object_to_object(struct dynamic_object* a, struct dynamic_object* b) {
    if (!(a->collision_layers & b->collision_layers)) {
        return;
    }

    if (a->is_trigger && b->is_trigger) {
        return;
    }

    struct Simplex simplex;
    if (!gjkCheckForOverlap(&simplex, a, dynamic_object_minkowski_sum, b, dynamic_object_minkowski_sum, &gRight)) {
        return;
    }

    struct EpaResult result;

    epaSolve(&simplex, a, dynamic_object_minkowski_sum, b, dynamic_object_minkowski_sum, &result);

    float friction = a->type->friction < b->type->friction ? a->type->friction : b->type->friction;
    float bounce = a->type->friction > b->type->friction ? a->type->friction : b->type->friction;

    // TODO determine push 
    if (!a->is_trigger && !b->is_trigger) {
        correct_overlap(b, &result, -0.5f, friction, bounce);
        correct_overlap(a, &result, 0.5f, friction, bounce);
    }

    struct contact* contact = collision_scene_new_contact();

    if (!contact) {
        return;
    }

    if (!a->is_trigger) {
        contact->normal = result.normal;
        contact->point = result.contactA;
        contact->other_object = a ? a->entity_id : 0;

        contact->next = b->active_contacts;
        b->active_contacts = contact;
        
        contact = collision_scene_new_contact();
    }

    if (!contact) {
        return;
    }

    if (!b->is_trigger) {
        vector3Negate(&result.normal, &contact->normal);
        contact->point = result.contactB;
        contact->other_object = b ? b->entity_id : 0;

        contact->next = a->active_contacts;
        a->active_contacts = contact;
    }
}