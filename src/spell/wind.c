#include "wind.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/shapes/cylinder_horz.h"
#include "../entity/entity_id.h"
#include "../collision/collision_scene.h"

#define WIND_SPEED   16.0f
#define WIND_ACCEL      10.0f

static struct wind_definition wind_definitions[] = {
    [ELEMENT_TYPE_NONE] = {
        .acceleration = 10.0f,
        .top_speed = 16.0f,
        .burst_time = 0.0f,
        .flags = 0,
    },
    [ELEMENT_TYPE_FIRE] = {
        .acceleration = 0.0f,
        .top_speed = 10.0f,
        .burst_time = 1.0f,
        .flags = 0,
    },
    [ELEMENT_TYPE_ICE] = {
        .acceleration = 16.0f,
        .top_speed = 10.0f,
        .burst_time = 0.0f,
        .flags = WIND_FLAGS_ICY,
    },
    [ELEMENT_TYPE_LIGHTNING] = {
        .acceleration = 0.0f,
        .top_speed = 100.0f,
        .burst_time = 0.1f,
        .flags = WIND_FLAGS_LIGHTNING,
    },
};

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

    wind->definition = effect_definition;
    wind->push_timer = effect_definition->burst_time;
    wind->flags = 0;
    wind->current_pushing_count = 0;

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
    wind->dynamic_object.collision_group = source->target;
    wind->dynamic_object.center = (struct Vector3){
        .x = 0.0f,
        .y = 0.0f,
        .z = wind_collider.data.cylinder.half_height,
    };

    dynamic_object_recalc_bb(&wind->dynamic_object);

    collision_scene_add(&wind->dynamic_object);
}

void wind_apply_burst_velocity(struct wind* wind, float speed) {
    struct Vector3 wind_direction;
    vector2ToLookDir(&wind->transform.rotation, &wind_direction);

    for (int i = 0; i < wind->current_pushing_count; i += 1) {
        struct dynamic_object* obj = collision_scene_find_object(wind->pushing_entities[i]);

        if (!obj) {
            continue;
        }

        struct Vector3 tangent;
        vector3ProjectPlane(&obj->velocity, &wind_direction, &tangent);
        vector3AddScaled(&tangent, &wind_direction, speed, &obj->velocity);
    }
}

void wind_destroy(struct wind* wind) {
    if (wind->definition->burst_time) {
        // burst pushes lose the velocity after the initial push
        
        for (int i = 0; i < wind->current_pushing_count; i += 1) {
            struct dynamic_object* obj = collision_scene_find_object(wind->pushing_entities[i]);

            if (!obj) {
                continue;
            }

            obj->velocity = gZeroVec;
        }
    }

    spell_data_source_release(wind->data_source);
    renderable_destroy(&wind->renderable);
    render_scene_remove(&wind->renderable);
    collision_scene_remove(&wind->dynamic_object);
}

bool wind_update_burst(struct wind* wind, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (!(wind->flags & WIND_FLAGS_DID_BURST)) {
        struct contact* contact = wind->dynamic_object.active_contacts;

        if (contact) {
            wind->flags |= WIND_FLAGS_DID_BURST;
            int contact_index = 0;
    
            while (contact && contact_index < MAX_PUSHING_ENTITIES) {
                wind->pushing_entities[contact_index] = contact->other_object;
                contact = contact->next;
                ++contact_index;
            }
    
            wind->current_pushing_count = contact_index;
            collision_scene_remove(&wind->dynamic_object);
        }
    }

    wind_apply_burst_velocity(wind, wind->definition->top_speed);

    wind->push_timer -= fixed_time_step;

    return wind->push_timer > 0.0f && wind->data_source->flags.cast_state != SPELL_CAST_STATE_INACTIVE;
}

bool wind_update_persistant(struct wind* wind, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    spell_data_source_apply_transform_sa(wind->data_source, &wind->transform);

    for (int i = 0; i < MAX_WIND_BONES && wind->renderable.armature.bone_count; i += 1) {
        struct Quaternion tmp;
        quatMultiply(&wind->renderable.armature.pose[i].rotation, &wind->bone_rotations[i], &tmp);
        wind->renderable.armature.pose[i].rotation = tmp;
    }

    struct contact* curr = wind->dynamic_object.active_contacts;

    struct Vector3 target_wind = {
        .x = wind->transform.rotation.y * WIND_SPEED,
        .y = 0.0f,
        .z = wind->transform.rotation.x * WIND_SPEED,
    };

    while (curr) {
        struct dynamic_object* obj = collision_scene_find_object(curr->other_object);
        curr = curr->next;

        if (!obj) {
            continue;
        }

        vector3MoveTowards(&obj->velocity, &target_wind, fixed_time_step * WIND_ACCEL, &obj->velocity);
        DYNAMIC_OBJECT_MARK_PUSHED(obj);

        if (wind->definition->flags & WIND_FLAGS_ICY) {
            DYNAMIC_OBJECT_MARK_DISABLE_FRICTION(obj);
        }

        obj->velocity.y -= (GRAVITY_CONSTANT - 0.1f) * fixed_time_step;
    }

    return wind->data_source->flags.cast_state == SPELL_CAST_STATE_ACTIVE;
}

bool wind_update(struct wind* wind, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (wind->definition->burst_time) {
        return wind_update_burst(wind, event_listener, spell_sources);
    } else {
        return wind_update_persistant(wind, event_listener, spell_sources);
    }
}

struct wind_definition* wind_lookup_definition(enum element_type element) {
    return &wind_definitions[element];
}