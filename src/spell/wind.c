#include "wind.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/shapes/cylinder_horz.h"
#include "../entity/entity_id.h"
#include "../collision/collision_scene.h"

static struct dynamic_object_type wind_collider = {
    .minkowsi_sum = cylinder_horz_minkowski_sum,
    .bounding_box = cylinder_horz_bounding_box,
    .data = {
        .cylinder = {
            .radius = 1.0f,
            .half_height = 2.0f,
        }
    }
};

static float radius_per_second[MAX_WIND_BONES] = {
    3.14f * 1.5f,
    -3.14f * 1.5f,
    3.14f * 2.0f,
};

void wind_init(struct wind* wind, struct spell_data_source* source, struct spell_event_options event_options, struct wind_definition* effect_definition) {
    entity_id id = entity_id_new();

    wind->data_source = spell_data_source_retain(source);
    spell_data_source_apply_transform_sa(source, &wind->transform);
    renderable_single_axis_init(&wind->renderable, &wind->transform, "rom:/meshes/spell/wind.tmesh");

    render_scene_add_renderable(&wind->renderable, 1.0f);

    for (int i = 0; i < MAX_WIND_BONES && wind->renderable.armature.bone_count; i += 1) {
        quatAxisAngle(&gUp, radius_per_second[i] * fixed_time_step, &wind->bone_rotations[i]);
    }

    dynamic_object_init(
        id, 
        &wind->dynamic_object, 
        &wind_collider, 
        COLLISION_LAYER_DAMAGE_ENEMY | COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE, 
        &wind->transform.position, 
        &wind->transform.rotation
    );

    wind->dynamic_object.is_trigger = 1;
    wind->dynamic_object.center = (struct Vector3){
        .x = 0.0f,
        .y = 0.0f,
        .z = -wind_collider.data.cylinder.half_height,
    };

    collision_scene_add(&wind->dynamic_object);
}

void wind_destroy(struct wind* wind) {
    spell_data_source_release(wind->data_source);
    renderable_destroy(&wind->renderable);
    render_scene_remove(&wind->renderable);
    collision_scene_remove(&wind->dynamic_object);
}

bool wind_update(struct wind* wind, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    spell_data_source_apply_transform_sa(wind->data_source, &wind->transform);

    for (int i = 0; i < MAX_WIND_BONES && wind->renderable.armature.bone_count; i += 1) {
        struct Quaternion tmp;
        quatMultiply(&wind->renderable.armature.pose[i].rotation, &wind->bone_rotations[i], &tmp);
        wind->renderable.armature.pose[i].rotation = tmp;
    }

    struct contact* curr = wind->dynamic_object.active_contacts;

    while (curr) {
        struct dynamic_object* obj = collision_scene_find_object(curr->other_object);
        curr = curr->next;

        if (!obj) {
            continue;
        }

        obj->position->x += 0.01f;
    }

    return wind->data_source->flags.cast_state == SPELL_CAST_STATE_ACTIVE;
}