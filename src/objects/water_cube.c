#include "water_cube.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"

#define OBJECT_DENSITY  0.6f

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

        float underwater_ratio = obj->bounding_box.max.y <= water_top ? 
            1.0f : 
            (water_top - obj->bounding_box.min.y) / (obj->bounding_box.max.y - obj->bounding_box.min.y);

        vector3Scale(&obj->velocity, &obj->velocity, 0.9f);
        obj->velocity.y -= underwater_ratio * (GRAVITY_CONSTANT / OBJECT_DENSITY) * fixed_time_step;

        if (underwater_ratio > 0.5f) {
            obj->under_water = 2;
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
    update_add(cube, water_cube_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
}

void water_cube_destroy(struct water_cube* cube) {
    collision_scene_remove_trigger(&cube->trigger);
    update_remove(cube);
}