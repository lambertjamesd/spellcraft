#include "cut_rope.h"

#include "../scene/scene.h"

void cut_rope_update(void* data) {
    cut_rope_t* cut_rope = (cut_rope_t*)data;

    dynamic_object_t* connected_to = collision_scene_find_object(scene_lookup_entity(current_scene, cut_rope->connected_to));

    if (connected_to == NULL) {
        entity_despawn(cut_rope->health.entity_id);
        return;
    } else {
        vector3_t connection_point = {
            (connected_to->bounding_box.max.x + connected_to->bounding_box.max.x) * 0.5,
            connected_to->bounding_box.max.y,
            (connected_to->bounding_box.max.z + connected_to->bounding_box.max.z) * 0.5,
        };

        vector3_t offset;
        vector3Sub(&connection_point, &cut_rope->position, &offset);
        float distance_sq = vector3MagSqrd(&offset);

        if (cut_rope->length == 0.0f) {
            cut_rope->length = sqrtf(distance_sq);
            cut_rope->last_tip = connection_point;
        } else {
            if (distance_sq > cut_rope->length * cut_rope->length) {
                vector3_t normal;
                vector3Scale(&offset, &normal, 1.0f / sqrtf(distance_sq));

                vector3AddScaled(&cut_rope->position, &normal, cut_rope->length, &cut_rope->last_tip);

                vector3_t move_by;
                vector3Sub(&cut_rope->last_tip, &connection_point, &move_by);
                vector3Add(connected_to->position, &move_by, connected_to->position);

                vector3ProjectPlane(&connected_to->velocity, &normal, &connected_to->velocity);
            } else {
                cut_rope->last_tip = connection_point;
            }
        }
    }

    if (!health_is_alive(&cut_rope->health)) {
        entity_despawn(cut_rope->health.entity_id);

        if (connected_to) {
            dynamic_object_wake(connected_to);
        }
        return;
    }

    swing_shape_add(&cut_rope->swing_shape, &cut_rope->position, &cut_rope->last_tip);
}
    
void cut_rope_init(cut_rope_t* cut_rope, struct cut_rope_definition* definition, entity_id entity_id) {
    cut_rope->position = definition->position;
    cut_rope->last_tip = definition->position;
    cut_rope->tip_velocity = gZeroVec;
    cut_rope->length = 0.0f;
    cut_rope->connected_to = definition->connected_to;

    swing_shape_init(&cut_rope->swing_shape, &cut_rope->collider_type);

    dynamic_object_init(
        entity_id,
        &cut_rope->collider,
        &cut_rope->collider_type,
        COLLISION_LAYER_TANGIBLE,
        &gZeroVec,
        NULL
    );

    cut_rope->collider.is_fixed = true;
    cut_rope->collider.weight_class = WEIGHT_CLASS_GHOST;

    collision_scene_add(&cut_rope->collider);

    update_add(cut_rope, cut_rope_update, UPDATE_PRIORITY_PHYICS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    health_init(&cut_rope->health, entity_id, 5.0f);
}

void cut_rope_destroy(cut_rope_t* cut_rope, struct cut_rope_definition* definition) {
    collision_scene_remove(&cut_rope->collider);
    health_destroy(&cut_rope->health);
}

void cut_rope_common_init() {

}

void cut_rope_common_destroy() {

}
