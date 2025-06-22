#include "collide.h"

#include "epa.h"

#include "collision_scene.h"
#include "../util/flags.h"
#include <stdio.h>


void correct_velocity(struct dynamic_object* object, struct Vector3* normal, float ratio, float friction, float bounce) {
    float velocityDot = vector3Dot(&object->velocity, normal);

    if (object->type->friction == 0.125f) {
        fprintf(stderr, "velocityDot = %f\n", velocityDot);
    }

    if ((velocityDot < 0) == (ratio < 0)) {
        struct Vector3 tangentVelocity;

        vector3AddScaled(&object->velocity, normal, -velocityDot, &tangentVelocity);
        vector3Scale(&tangentVelocity, &tangentVelocity, 1.0f - friction);

        if (object->type->friction == 0.125f) {
            fprintf(stderr, "friction = %f, x,y,z=%f,%f,%f\n", friction, tangentVelocity.x, tangentVelocity.y, tangentVelocity.z);
        }

        vector3AddScaled(&tangentVelocity, normal, velocityDot * -bounce, &object->velocity);
    }
}

void correct_overlap_no_slide(
    struct dynamic_object* object, 
    float penetration, 
    float slope_amount, 
    float ratio, 
    float friction, 
    float bounce
) {
    float offset = penetration / slope_amount;
    object->position->y -= offset;

    if (object->velocity.y < 0.0f) {
        object->velocity.y = -object->velocity.y * object->type->bounce;
    }

    if (object->type->friction) {
        float scalar = 1.0f - object->type->friction;
        object->velocity.x *= scalar;
        object->velocity.z *= scalar;
    }
}

void correct_overlap(struct dynamic_object* object, struct EpaResult* result, float ratio, float friction, float bounce) {
    if (object->is_fixed) {
        return;
    }

    float slope_amount = ratio > 0.0f ? -result->normal.y : result->normal.y;

    if (dynamic_object_should_slide(object->type->max_stable_slope, slope_amount)) {
        vector3AddScaled(object->position, &result->normal, result->penetration * ratio, object->position);
        correct_velocity(object, &result->normal, ratio, friction, bounce);
    } else {
        correct_overlap_no_slide(
            object,
            result->penetration,
            slope_amount,
            ratio,
            friction,
            bounce
        );
    }

}

struct object_mesh_collide_data {
    struct mesh_collider* mesh;
    struct dynamic_object* object;
    struct mesh_triangle triangle;
};

bool collide_object_to_triangle(struct mesh_index* index, void* data, int triangle_index) {
    struct object_mesh_collide_data* collide_data = (struct object_mesh_collide_data*)data;
    collide_data->triangle.triangle = collide_data->mesh->triangles[triangle_index];

    struct Simplex simplex;
    if (!gjkCheckForOverlap(&simplex, &collide_data->triangle, mesh_triangle_minkowski_sum, collide_data->object, dynamic_object_minkowski_sum, &gRight)) {
        return false;
    }

    struct EpaResult result;

    if (epaSolve(
        &simplex, 
        &collide_data->triangle, 
        mesh_triangle_minkowski_sum, 
        collide_data->object, 
        dynamic_object_minkowski_sum, 
        &result)) {
        correct_overlap(
            collide_data->object, 
            &result, 
            -1.0f, 
            collide_data->object->disable_friction ? 0.0f : collide_data->object->type->friction, 
            collide_data->object->type->bounce
        );
        collide_add_contact(collide_data->object, &result);
        return true;
    }

    return false;
}

void collide_object_to_mesh(struct dynamic_object* object, struct mesh_collider* mesh) {   
    if (object->is_trigger) {
        return;
    }

    struct object_mesh_collide_data collide_data;
    collide_data.mesh = mesh;
    collide_data.object = object;
    collide_data.triangle.vertices = mesh->vertices;
    mesh_index_lookup_triangle_indices(&mesh->index, &object->bounding_box, collide_object_to_triangle, &collide_data);
}

void collide_object_to_object(struct dynamic_object* a, struct dynamic_object* b) {
    if (!(a->collision_layers & b->collision_layers)) {
        return;
    }

    if (a->collision_group && a->collision_group == b->collision_group) {
        return;
    }

    if (a->is_trigger && b->is_trigger) {
        return;
    }

    struct Simplex simplex;
    if (!gjkCheckForOverlap(&simplex, a, dynamic_object_minkowski_sum, b, dynamic_object_minkowski_sum, &gRight)) {
        return;
    }

    if (a->is_trigger || b->is_trigger) {
        struct contact* contact = collision_scene_new_contact();

        if (!contact) {
            return;
        }

        if (b->is_trigger) {
            contact->normal = gZeroVec;
            contact->point = *a->position;
            contact->other_object = a ? a->entity_id : 0;

            contact->next = b->active_contacts;
            b->active_contacts = contact;
        } else {
            contact->normal = gZeroVec;
            contact->point = *b->position;
            contact->other_object = b ? b->entity_id : 0;

            contact->next = a->active_contacts;
            a->active_contacts = contact;
        }

        return;
    }

    struct EpaResult result;

    if (!epaSolve(&simplex, a, dynamic_object_minkowski_sum, b, dynamic_object_minkowski_sum, &result)) {
        return;
    }

    float friction = a->type->friction < b->type->friction ? a->type->friction : b->type->friction;
    float bounce = a->type->friction > b->type->friction ? a->type->friction : b->type->friction;

    // TODO better ratio depending on object weights
    float overlap_ratio = 0.5f;

    correct_overlap(b, &result, overlap_ratio - 1.0f, b->disable_friction ? 0.0f : friction, bounce);
    correct_overlap(a, &result, overlap_ratio, a->disable_friction ? 0.0f : friction, bounce);

    struct contact* contact = collision_scene_new_contact();

    if (!contact) {
        return;
    }

    contact->normal = result.normal;
    contact->point = result.contactA;
    contact->other_object = a ? a->entity_id : 0;

    contact->next = b->active_contacts;
    b->active_contacts = contact;

    contact = collision_scene_new_contact();

    if (!contact) {
        return;
    }
    
    vector3Negate(&result.normal, &contact->normal);
    contact->point = result.contactB;
    contact->other_object = b ? b->entity_id : 0;

    contact->next = a->active_contacts;
    a->active_contacts = contact;
}

void collide_object_to_trigger(struct dynamic_object* obj, struct spatial_trigger* trigger) {
    if (!spatial_trigger_does_contain_point(trigger, obj->position)) {
        return;
    }

    struct contact* contact = collision_scene_new_contact();

    if (!contact) {
        return;
    }
    
    contact->normal = gZeroVec;
    contact->point = *obj->position;
    contact->other_object = obj->entity_id;

    contact->next = trigger->active_contacts;
    trigger->active_contacts = contact;
}

void collide_add_contact(struct dynamic_object* object, struct EpaResult* result) {
    struct contact* contact = collision_scene_new_contact();

    if (!contact) {
        return;
    }

    contact->normal = result->normal;
    contact->point = result->contactA;
    contact->other_object = 0;

    contact->next = object->active_contacts;
    object->active_contacts = contact;
}