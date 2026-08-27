#include "pulley_gate.h"    

#include "../collision/shapes/box.h"

static dynamic_object_type_t gate_collider = {
    BOX_COLLIDER(1.0f, 1.5f, 0.5f),
    .center = {0.0f, 1.5f, 0.0f},
};

static dynamic_object_type_t harness_collider = {
    BOX_COLLIDER(1.0f, 0.1f, 1.0f),
    .center = {0.0f, -0.1f, 0.0f},
};

#define ACCEL               3.0f
#define WEIGHT_THRESHOLD    1.0f
#define MAX_OFFSET          1.8f

#define GATE_BONE_INDEX     0
#define HARNESS_BONE_INDEX  1

static float balance_weights[] = {
    [WEIGHT_CLASS_GHOST] = 0.0f,
    [WEIGHT_CLASS_LIGHT] = 0.4f,
    [WEIGHT_CLASS_MEDIUM] = 1.0f,
    [WEIGHT_CLASS_HEAVY] = 2.0f,
    [WEIGHT_CLASS_SUPER_HEAVY] = 4.0f,
};

float pulley_gate_get_weight(pulley_gate_t* pulley_gate) {
    float weight = 0.0f;

    for (
        contact_t* contact = pulley_gate->harness_collider.active_contacts;
        contact;
        contact = contact->next
    ) {
        if (contact->normal.y > -0.5f) {
            continue;
        }

        dynamic_object_t* obj = collision_scene_find_object(contact->other_object);

        if (!obj) {
            continue;
        }

        weight += balance_weights[obj->weight_class];
    }

    return weight;
}

void pulley_gate_update(void* data) {
    pulley_gate_t* pulley_gate = (pulley_gate_t*)data;

    float target_offset = 0.0f;

    if (pulley_gate_get_weight(pulley_gate) > WEIGHT_THRESHOLD) {
        target_offset = -MAX_OFFSET;
    } 

    float current_offset = pulley_gate->harness_position.y - pulley_gate->harness_start_y;

    expression_set_bool(pulley_gate->output, current_offset < -MAX_OFFSET * 0.5f);

    if (current_offset != target_offset) {
        if (target_offset > current_offset) {
            pulley_gate->velocity += ACCEL * fixed_time_step;
        } else {
            pulley_gate->velocity -= ACCEL * fixed_time_step;
        }

        current_offset += pulley_gate->velocity * fixed_time_step;

        if (current_offset > 0.0f) {
            current_offset = 0.0f;
            pulley_gate->velocity = 0.0f;
        } else if (current_offset < -MAX_OFFSET) {
            current_offset = -MAX_OFFSET;
            pulley_gate->velocity = 0.0f;
        }

        pulley_gate->harness_position.y = pulley_gate->harness_start_y + current_offset;
        pulley_gate->harness_collider.velocity.y = pulley_gate->velocity;
        pulley_gate->gate_position.y = pulley_gate->gate_start_y - current_offset;
        pulley_gate->gate_collider.velocity.y = -pulley_gate->velocity;

        transform_t* pose = pulley_gate->renderable.mesh_render.armature.pose;

        pose[HARNESS_BONE_INDEX].position.y = (pulley_gate->harness_position.y - pulley_gate->gate_transform.position.y) * MODEL_SCALE;
        pose[GATE_BONE_INDEX].position.y = (pulley_gate->gate_position.y - pulley_gate->gate_transform.position.y) * MODEL_SCALE;
    }
}
    
void pulley_gate_init(pulley_gate_t* pulley_gate, struct pulley_gate_definition* definition, entity_id entity_id) {
    transformSaInit(&pulley_gate->gate_transform, &definition->position, &definition->rotation, 1.0f);
    pulley_gate->gate_position = definition->position;
    pulley_gate->harness_position = definition->harness_position;
    pulley_gate->gate_start_y = definition->position.y;
    pulley_gate->harness_start_y = definition->harness_position.y;
    pulley_gate->output = definition->output;

    pulley_gate->velocity = 0.0f;

    float radius = sqrtf(vector3DistSqrd(&definition->position, &definition->harness_position));

    renderable_single_axis_init(&pulley_gate->renderable, &pulley_gate->gate_transform, "rom:/meshes/puzzle/pulley_gate.tmesh");
    render_scene_add_renderable(&pulley_gate->renderable, radius + 4.0);

    dynamic_object_init(
        entity_id, 
        &pulley_gate->gate_collider, 
        &gate_collider, 
        COLLISION_LAYER_TANGIBLE, 
        &pulley_gate->gate_position, 
        &pulley_gate->gate_transform.rotation
    );
    pulley_gate->gate_collider.is_fixed = true;
    pulley_gate->gate_collider.weight_class = WEIGHT_CLASS_HEAVY;
    collision_scene_add(&pulley_gate->gate_collider);
    
    dynamic_object_init(
        entity_id, 
        &pulley_gate->harness_collider, 
        &harness_collider, 
        COLLISION_LAYER_TANGIBLE, 
        &pulley_gate->harness_position, 
        &pulley_gate->gate_transform.rotation
    );
    pulley_gate->harness_collider.is_fixed = true;
    pulley_gate->harness_collider.weight_class = WEIGHT_CLASS_HEAVY;
    collision_scene_add(&pulley_gate->harness_collider);

    update_add(pulley_gate, pulley_gate_update, UPDATE_PRIORITY_PHYICS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
}

void pulley_gate_destroy(pulley_gate_t* pulley_gate, struct pulley_gate_definition* definition) {
    renderable_destroy(&pulley_gate->renderable);
    render_scene_remove(&pulley_gate->renderable);

    collision_scene_remove(&pulley_gate->gate_collider);
    collision_scene_remove(&pulley_gate->harness_collider);

    update_remove(pulley_gate);
}

void pulley_gate_common_init() {

}

void pulley_gate_common_destroy() {

}
