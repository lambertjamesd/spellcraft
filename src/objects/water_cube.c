#include "water_cube.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"

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

        obj->under_water = 2;
        vector3Scale(&obj->velocity, &obj->velocity, 0.9f);
        obj->velocity.y -= GRAVITY_CONSTANT * fixed_time_step;
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