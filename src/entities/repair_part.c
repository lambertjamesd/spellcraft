#include "repair_part.h"    

#include "../screen_raycast/screen_raycast.h"
#include "../util/hash_map.h"
#include "../scene/scene.h"

static hash_map_t active_repair_parts;
static material_pair_t* missing_material;
static material_pair_t* correct_slot_material;

void repair_part_do_render(void* data, struct render_batch* batch) {
    repair_part_t* repair_part = (repair_part_t*)data;

    T3DMat4FP* mtxfp = render_batch_transformfp_from_full(batch, &repair_part->transform);

    if (!mtxfp) {
        return;
    }
    
    if (!repair_part->is_connected && repair_part->is_present) {
        screen_raycast_check_after(0);
    }

    t3d_matrix_push(mtxfp);

    rspq_block_run(repair_part->mesh->block);

    t3d_matrix_pop(1);

    if (!repair_part->is_connected && repair_part->is_present) {
        screen_raycast_check_after(repair_part->entity_id);
    }
}


void repair_part_render_drop_location(repair_part_t* part, struct render_batch* batch) {
    T3DMat4FP* mtxfp = render_batch_transformfp_from_full(batch, &part->end_transform);

    if (!mtxfp) {
        return;
    }

    render_batch_element_t* element = render_batch_add_tmesh(
        batch,
        part->mesh,
        mtxfp,
        NULL,
        NULL,
        NULL
    );

    element->material = correct_slot_material;
}

void repair_part_render(void* data, struct render_batch* batch) {
    repair_part_t* repair_part = (repair_part_t*)data;

    render_batch_add_callback(batch, repair_part->is_present ? repair_part->mesh->material : missing_material, repair_part_do_render, repair_part);
}
    
void repair_part_init(repair_part_t* repair_part, struct repair_part_definition* definition, entity_id entity_id) {
    transformInit(&repair_part->transform, &definition->start_position, &definition->start_rotation, &gOneVec);
    transformInit(&repair_part->end_transform, &definition->position, &definition->rotation, &gOneVec);
    hash_map_set(&active_repair_parts, entity_id, repair_part);

    repair_part->mesh = tmesh_cache_load(definition->mesh);
    render_scene_add(&repair_part->transform.position, 4.0f, repair_part_render, repair_part);

    repair_part->entity_id = entity_id;
    repair_part->depends_on = definition->depends_on;
    repair_part->prevent_rotation = definition->prevent_rotation;
    repair_part->is_present = definition->has_part == VARIABLE_DISCONNECTED ? true : expression_get_bool(definition->has_part);
    repair_part->is_connected = false;
}

void repair_part_destroy(repair_part_t* repair_part, struct repair_part_definition* definition) {
    render_scene_remove(repair_part);
    tmesh_cache_release(repair_part->mesh);
    hash_map_delete(&active_repair_parts, repair_part->entity_id);
}

void repair_part_common_init() {
    hash_map_init(&active_repair_parts, 8);
    missing_material = material_cache_load("rom:/materials/repair/part_missing.mat");
    correct_slot_material = material_cache_load("rom:/materials/repair/correct_slot.mat");
}

void repair_part_common_destroy() {
    hash_map_destroy(&active_repair_parts);
    material_cache_release(missing_material);
    material_cache_release(correct_slot_material);
}

void repair_part_connect(repair_part_t* repair_part) {
    repair_part->transform = repair_part->end_transform;
    repair_part->is_connected = true;
}

bool repair_part_can_connect(repair_part_t* repair_part) {
    if (repair_part->depends_on == ENTITY_SPAWNER_UNLINKED) {
        return true;
    }

    repair_part_t* other_part = repair_part_get(scene_lookup_entity(current_scene, repair_part->depends_on));
    return other_part->is_connected;
}

repair_part_t* repair_part_get(entity_id id) {
    return hash_map_get(&active_repair_parts, id);
}

enum repair_part_status repair_part_status() {
    for (
        struct hash_map_entry* entry = hash_map_next(&active_repair_parts, NULL);
        entry;
        entry = hash_map_next(&active_repair_parts, entry)
    ) {
        repair_part_t* repair_part = (repair_part_t*)entry->value;

        if (!repair_part->is_present) {
            return REPAIR_MISSING_PARTS;
        }

        if (!repair_part->is_connected) {
            return REPAIR_INCOMPLETE;
        }
    }

    return REPAIR_COMPLETE;
}

#define DROP_TOLERNACE  1.0f

bool repair_part_is_in_right_spot(repair_part_t* repair_part, ray_t* ray_check) {
    if (fabsf(quatDot(&repair_part->transform.rotation, &repair_part->end_transform.rotation)) < 0.9f) {
        return false;
    }

    float target_distance = rayDetermineDistance(ray_check, &repair_part->end_transform.position);
    float actual_distnace = rayDetermineDistance(ray_check, &repair_part->transform.position);

    vector3_t pos_check;
    vector3AddScaled(&repair_part->transform.position, &ray_check->dir, target_distance - actual_distnace, &pos_check);

    return vector3DistSqrd(&pos_check, &repair_part->end_transform.position) <= DROP_TOLERNACE * DROP_TOLERNACE;
}