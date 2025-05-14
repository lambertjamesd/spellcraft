#include "wind.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/shapes/cylinder_horz.h"
#include "../collision/shapes/cylinder.h"
#include "../entity/entity_id.h"
#include "../collision/collision_scene.h"

#define WIND_SPEED      16.0f
#define WIND_ACCEL      10.0f

static struct wind_definition wind_definitions[] = {
    [ELEMENT_TYPE_NONE] = {
        .push = {
            .acceleration = 10.0f,
            .top_speed = 16.0f,
            .time = 0.0f,
            .bursty = false,
        },
        .base_scale = 1.0f,
    },
    [ELEMENT_TYPE_FIRE] = {
        .push = {
            .acceleration = 0.0f,
            .top_speed = 10.0f,
            .time = 1.0f,
            .bursty = true,
        },
        .base_scale = 1.0f,
    },
    [ELEMENT_TYPE_ICE] = {
        .push = {
            .acceleration = 16.0f,
            .top_speed = 10.0f,
            .time = 0.0f,
            .bursty = false,
        },
        .base_scale = 1.0f,
        .icy = true,
    },
    [ELEMENT_TYPE_LIGHTNING] = {
        .push = {
            .acceleration = 0.0f,
            .top_speed = 100.0f,
            .time = 0.1f,
            .bursty = true,
            .lightning = true,
        },
        .base_scale = 1.0f,
        .lightning = true,
    },
};

static struct wind_definition wind_sphere_definitions[] = {
    [ELEMENT_TYPE_NONE] = {
        .push = {
            .acceleration = 10.0f,
            .top_speed = 16.0f,
            .time = 0.0f,
        },
        .base_scale = 1.0f,
        .sphere = true,
    },
    [ELEMENT_TYPE_FIRE] = {
        .push = {
            .acceleration = 0.0f,
            .top_speed = 10.0f,
            .time = 0.5f,
            .bursty = true,
        },
        .base_scale = 1.0f,
        .sphere = true,
    },
    [ELEMENT_TYPE_ICE] = {
        .push = {
            .acceleration = 16.0f,
            .top_speed = 10.0f,
            .time = 0.0f,
        },
        .base_scale = 1.0f,
        .sphere = true,
        .icy = true,
    },
    [ELEMENT_TYPE_LIGHTNING] = {
        .push = {
            .acceleration = 0.0f,
            .top_speed = 100.0f,
            .time = 0.1f,
            .bursty = true,
            .lightning = true,
        },
        .base_scale = 1.0f,
        .sphere = true,
        .lightning = true,
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

static struct dynamic_object_type wind_collider_sphere = {
    .minkowsi_sum = cylinder_minkowski_sum,
    .bounding_box = cylinder_bounding_box,
    .data = {
        .cylinder = {
            .radius = 2.0f,
            .half_height = 1.0f,
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
    wind->push_timer = effect_definition->push.time;
    wind->did_burst = false;

    wind->data_source = spell_data_source_retain(source);
    spell_data_source_apply_transform_sa(source, &wind->transform);
    renderable_single_axis_init(&wind->renderable, &wind->transform, effect_definition->sphere ? "rom:/meshes/spell/wind_sphere.tmesh" : "rom:/meshes/spell/wind.tmesh");

    render_scene_add_renderable(&wind->renderable, 1.0f);

    for (int i = 0; i < MAX_WIND_BONES && wind->renderable.armature.bone_count; i += 1) {
        quatAxisAngle(&gUp, radius_per_second[i] * fixed_time_step, &wind->bone_rotations[i]);
    }

    dynamic_object_init(
        id, 
        &wind->dynamic_object, 
        effect_definition->sphere ? &wind_collider_sphere : &wind_collider, 
        COLLISION_LAYER_DAMAGE_ENEMY | COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE, 
        &wind->transform.position, 
        &wind->transform.rotation
    );

    wind->dynamic_object.is_trigger = 1;
    wind->dynamic_object.collision_group = source->target;

    if (!effect_definition->sphere) {
        wind->dynamic_object.center = (struct Vector3){
            .x = 0.0f,
            .y = 0.0f,
            .z = wind_collider.data.cylinder.half_height,
        };
    }

    dynamic_object_recalc_bb(&wind->dynamic_object);

    collision_scene_add(&wind->dynamic_object);
}

void wind_apply_push_velocity_with_dir(struct wind_definition* definition, struct dynamic_object* obj, struct Vector3* wind_direction)  {
    single_push_apply_velocity_with_dir(&definition->push, obj, wind_direction);
    if (definition->icy) {
        DYNAMIC_OBJECT_MARK_DISABLE_FRICTION(obj);
    }
}

void wind_apply_push_velocity(struct wind* wind) {
    struct Vector3 wind_direction;
    vector2ToLookDir(&wind->transform.rotation, &wind_direction);

    for (
        struct contact* contact = wind->dynamic_object.active_contacts; 
        contact; 
        contact = contact->next) {
        struct dynamic_object* obj = collision_scene_find_object(contact->other_object);

        if (!obj) {
            continue;
        }

        wind_apply_push_velocity_with_dir(wind->definition, obj, &wind_direction);
    }
}

void wind_apply_sphere_push_velocity(struct wind* wind) {
    for (
        struct contact* contact = wind->dynamic_object.active_contacts; 
        contact; 
        contact = contact->next) {
        struct dynamic_object* obj = collision_scene_find_object(contact->other_object);

        if (!obj) {
            continue;
        }
        struct Vector3 wind_direction;
        vector3Sub(obj->position, &wind->transform.position, &wind_direction);
        wind_direction.y = 0.0f;
        vector3Normalize(&wind_direction, &wind_direction);

        wind_apply_push_velocity_with_dir(wind->definition, obj, &wind_direction);
    }
}

void wind_destroy(struct wind* wind) {
    spell_data_source_release(wind->data_source);
    renderable_destroy(&wind->renderable);
    render_scene_remove(&wind->renderable);
    collision_scene_remove(&wind->dynamic_object);
}

bool wind_update_burst(struct wind* wind, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (!wind->did_burst) {
        struct contact* contact = wind->dynamic_object.active_contacts;

        if (contact) {
            wind->did_burst = true;

            struct Vector3 wind_direction;
            if (!wind->definition->sphere) {
                vector2ToLookDir(&wind->transform.rotation, &wind_direction);
            }
    
            for (; contact; contact = contact->next) {
                if (wind->definition->sphere) {
                    struct dynamic_object* obj = collision_scene_find_object(contact->other_object);

                    if (!obj) {
                        continue;
                    }

                    vector3Sub(obj->position, &wind->transform.position, &wind_direction);
                    wind_direction.y = 0.0f;
                    vector3Normalize(&wind_direction, &wind_direction);
                }

                single_push_new(contact->other_object, &wind_direction, &wind->definition->push);
            }
        }
    }

    wind->push_timer -= fixed_time_step;

    return wind->push_timer > 0.0f || wind->data_source->flags.cast_state != SPELL_CAST_STATE_INACTIVE;
}

bool wind_update_persistant(struct wind* wind, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    spell_data_source_apply_transform_sa(wind->data_source, &wind->transform);

    if (wind->definition->sphere) {
        wind_apply_sphere_push_velocity(wind);
    } else {
        wind_apply_push_velocity(wind);
    }

    return wind->data_source->flags.cast_state == SPELL_CAST_STATE_ACTIVE;
}

bool wind_update(struct wind* wind, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    for (int i = 0; i < MAX_WIND_BONES && wind->renderable.armature.bone_count; i += 1) {
        struct Quaternion tmp;
        quatMultiply(&wind->renderable.armature.pose[i].rotation, &wind->bone_rotations[i], &tmp);
        wind->renderable.armature.pose[i].rotation = tmp;
    }

    if (wind->definition->push.bursty) {
        return wind_update_burst(wind, event_listener, spell_sources);
    } else {
        return wind_update_persistant(wind, event_listener, spell_sources);
    }
}

struct wind_definition* wind_lookup_definition(enum element_type element, bool has_ground) {
    return has_ground ? &wind_sphere_definitions[element] : &wind_definitions[element];
}