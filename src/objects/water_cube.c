#include "water_cube.h"

#include "../collision/collision_scene.h"
#include "../collision/collide.h"
#include "../time/time.h"
#include "../math/mathf.h"

#define OBJECT_DENSITY      0.6f
#define MAX_MOVE_AMOUNT     10.0f

void water_cube_update(void* data) {
    struct water_cube* cube = (struct water_cube*)data;

    for (
        struct contact* current = cube->trigger.active_contacts;
        current;
        current = current->next
    ) {
        struct dynamic_object* obj = collision_scene_find_object(current->other_object);

        if (!obj) {
            continue;
        }

        float water_top = cube->trigger.bounding_box.max.y;

        if (obj->bounding_box.min.y >= water_top) {
            continue;
        }

        if (obj->has_ice_dash) {
            struct contact* other = collision_scene_new_contact();

            if (!other) {
                continue;
            }

            other->next = obj->active_contacts;
            other->normal = gUp;
            other->other_object = 0;
            other->point = *obj->position;
            other->point.y = water_top;

            obj->active_contacts = other;

            float target_pos = water_top + obj->position->y - obj->bounding_box.min.y;

            obj->position->y = mathfMoveTowards(obj->position->y, target_pos, MAX_MOVE_AMOUNT * fixed_time_step);
            correct_velocity(obj, &gUp, -1.0f, 0.01f, 0.0f);

            continue;   
        }

        float underwater_ratio = obj->bounding_box.max.y <= water_top ? 
            1.0f : 
            (water_top - obj->bounding_box.min.y) / (obj->bounding_box.max.y - obj->bounding_box.min.y);

        vector3Scale(&obj->velocity, &obj->velocity, 0.9f);
        obj->velocity.y -= underwater_ratio * (GRAVITY_CONSTANT / OBJECT_DENSITY) * fixed_time_step;

        if (underwater_ratio > 0.5f) {
            DYNAMIC_OBJECT_MARK_UNDER_WATER(obj);
        }
    }
}

void water_cube_init(struct water_cube* cube, struct water_cube_definition* definition) {
    cube->transform.position = definition->position;
    cube->transform.rotation = definition->rotation;
    cube->trigger_type = (struct spatial_trigger_type){
        SPATIAL_TRIGGER_BOX(definition->scale.x, definition->scale.y, definition->scale.z)
    };
    spatial_trigger_init(&cube->trigger, &cube->transform, &cube->trigger_type);
    collision_scene_add_trigger(&cube->trigger);
    update_add(cube, water_cube_update, UPDATE_PRIORITY_PHYICS, UPDATE_LAYER_WORLD);
}

void water_cube_destroy(struct water_cube* cube) {
    collision_scene_remove_trigger(&cube->trigger);
    update_remove(cube);
}