#include "breakable.h"

#include "../collision/shapes/cylinder.h"
#include "../entity/entity_spawner.h"

struct breakable_type_def {
    const char* mesh_name;
    const char* break_mesh;
    const char* break_animations;
    float health;
    dynamic_object_type_t collider;
    uint8_t weight;
    uint8_t can_pickup;
};

typedef struct breakable_type_def breakable_type_def_t;

static breakable_type_def_t breakable_definitions[BREAKABLE_TYPE_COUNT] = {
    [BREAKABLE_FIRE_POT_MED] = {
        .mesh_name = "rom:/meshes/breakables/fire_pot_med.tmesh",
        .break_mesh = "rom:/meshes/breakables/fire_pot_med_break.tmesh",
        .break_animations = "rom:/meshes/breakables/fire_pot_med_break.anim",

        .health = 10.0f,

        .collider = {
            CYLINDER_COLLIDER(0.5f, 0.5f),
            .center = {0.0f, 0.5f, 0.0f},
            .friction = 0.9f,
            .bounce = 0.1f,
        },

        .weight = WEIGHT_CLASS_HEAVY,
        .can_pickup = true,
    }
};

void breakable_update(void* data) {
    breakable_t* breakable = (breakable_t*)data;

    animator_update(&breakable->animator, fixed_time_step);

    if (breakable->is_breaking) {
        if (!animator_is_running(&breakable->animator)) {
            entity_despawn(breakable->collider.entity_id);
        }
    } else {
        if (!health_is_alive(&breakable->health)) {
            breakable->is_breaking = true;

            if (breakable->break_effect_mesh) {
                renderable_set_mesh_direct(&breakable->renderable, breakable->break_effect_mesh);
                animator_init(&breakable->animator, breakable->break_effect_mesh->armature.bone_count);
                animator_run_clip(&breakable->animator, animation_set_find_clip(breakable->break_animations, "break"), 0.0f, false);
                renderable_set_animator(&breakable->renderable, &breakable->animator);
            }
        }
    }
}

void breakable_init(breakable_t* breakable, struct breakable_definition* definition, entity_id entity_id) {
    transformSaInit(&breakable->transform, &definition->position, &definition->rotation, 1.0f);
    
    assert(definition->breakable_type >= 0 && definition->breakable_type < BREAKABLE_TYPE_COUNT);
    breakable_type_def_t* breakable_def = &breakable_definitions[definition->breakable_type];

    breakable->mesh = tmesh_cache_load(breakable_def->mesh_name);
    breakable->break_effect_mesh = breakable_def->break_mesh ? tmesh_cache_load(breakable_def->break_mesh) : NULL;
    breakable->break_animations = breakable_def->break_animations ? animation_cache_load(breakable_def->break_animations) : NULL;
    render_scene_init_add_renderable(&breakable->renderable, &breakable->transform, breakable->mesh, 0.0f);
    animator_init(&breakable->animator, 0);

    health_init(&breakable->health, entity_id, breakable_def->health);

    dynamic_object_init(
        entity_id, 
        &breakable->collider, 
        &breakable_def->collider, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY, 
        &breakable->transform.position, 
        &breakable->transform.rotation
    );
    breakable->collider.weight_class = breakable_def->weight;

    collision_scene_add(&breakable->collider);

    update_add(breakable, breakable_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);

    breakable->is_breaking = false;

    interactable_init(&breakable->interactable, entity_id, breakable_def->can_pickup ? INTERACT_TYPE_PICKUP : INTERACTION_NONE, NULL, NULL);
}

void breakable_destroy(breakable_t* breakable, struct breakable_definition* definition) {
    render_scene_remove_renderable(&breakable->renderable);
    collision_scene_remove(&breakable->collider);
    health_destroy(&breakable->health);
    update_remove(breakable);
    animator_destroy(&breakable->animator);
    interactable_destroy(&breakable->interactable);

    tmesh_cache_release(breakable->mesh);

    if (breakable->break_effect_mesh) {
        tmesh_cache_release(breakable->break_effect_mesh);
    }
    
    if (breakable->break_animations) {
        animation_cache_release(breakable->break_animations);
    }
}

void breakable_common_init() {

}

void breakable_common_destroy() {

}
