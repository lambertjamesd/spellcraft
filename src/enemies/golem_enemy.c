#include "golem_enemy.h"    

#include "../math/constants.h"

#define VISION_DISTANCE 8.0f

static tmesh_t* golem_pot;
static tmesh_t* golem_full;

enum golem_animations {
    GOLEM_ANIM_WAKE_UP,

    GOLEM_ANIM_COUNT,
};

static animation_set_t* golem_animation_set;
static animation_clip_t* golem_animations[GOLEM_ANIM_COUNT];

static const char* golem_animation_names[GOLEM_ANIM_COUNT] = {
    [GOLEM_ANIM_WAKE_UP] = "wake_up",
};

static spatial_trigger_type_t golem_vision = {
    .type = SPATIAL_TRIGGER_WEDGE,
    .data = {
        .wedge = {
            .radius = VISION_DISTANCE,
            .half_height = VISION_DISTANCE,
            .angle = {SQRT_1_2, SQRT_1_2},
        },
    },
};

void golem_enemy_render(void* data, struct render_batch* batch) {
    golem_enemy_t* golem = (golem_enemy_t*)data;
    T3DMat4FP* mtx = render_batch_transformfp_from_sa(batch, &golem->transform);

    if (!mtx) {
        return;
    }

    if (!golem->is_active) {
        render_batch_add_tmesh(batch, golem_pot, mtx, NULL, NULL, NULL);
        return;
    }

    animator_apply(&golem->animator, &golem->renderable.mesh_render.armature);
    
    render_batch_add_tmesh(batch, golem_full, mtx, &golem->renderable.mesh_render.armature, NULL, NULL);
}

void golem_enemy_update(void* data) {
    golem_enemy_t* golem_enemy = (golem_enemy_t*)data;

    animator_update(&golem_enemy->animator, fixed_time_step);

    vector3_t offset;
    dynamic_object_t* target = vision_update_current_target(&golem_enemy->target, &golem_enemy->vision, VISION_DISTANCE, &offset);

    debugf("vision = %d\n", (int)golem_enemy->vision.active_contacts);

    if (target) {
        if (!golem_enemy->is_active) {
            animator_run_clip(&golem_enemy->animator, golem_animations[GOLEM_ANIM_WAKE_UP], 0.0f, false);
        }

        golem_enemy->is_active = true;
    } else {
        golem_enemy->is_active = false;
    }
}

void golem_enemy_init(golem_enemy_t* golem_enemy, struct golem_enemy_definition* definition, entity_id entity_id) {
    transformSaInit(&golem_enemy->transform, &definition->position, &definition->rotation, 1.0f);

    renderable_single_axis_init_direct(&golem_enemy->renderable, &golem_enemy->transform, golem_full);
    render_scene_add(&golem_enemy->transform.position, 1.5f, golem_enemy_render, golem_enemy);
    animator_init(&golem_enemy->animator, golem_full->armature.bone_count);

    spatial_trigger_init(&golem_enemy->vision, &golem_enemy->transform, &golem_vision, COLLISION_LAYER_DAMAGE_PLAYER, entity_id);
    collision_scene_add_trigger(&golem_enemy->vision);

    golem_enemy->is_active = false;
    golem_enemy->activated = definition->activated;
    golem_enemy->target = 0;

    update_add(golem_enemy, golem_enemy_update, UPDATE_PRIORITY_ENEMY, UPDATE_LAYER_WORLD);
}

void golem_enemy_destroy(golem_enemy_t* golem_enemy, struct golem_enemy_definition* definition) {
    renderable_destroy_direct(&golem_enemy->renderable);
    render_scene_remove(&golem_enemy->renderable);
    update_remove(golem_enemy);
    collision_scene_remove_trigger(&golem_enemy->vision);
}

void golem_enemy_common_init() {
    golem_pot = tmesh_cache_load("rom:/meshes/enemies/golem_pot.tmesh");
    golem_full = tmesh_cache_load("rom:/meshes/enemies/golem.tmesh");

    golem_animation_set = animation_cache_load("rom:/meshes/enemies/golem.anim");

    for (int i = 0; i < GOLEM_ANIM_COUNT; i += 1) {
        golem_animations[i] = animation_set_find_clip(golem_animation_set, golem_animation_names[i]);
    }
}

void golem_enemy_common_destroy() {
    tmesh_cache_release(golem_pot);
    tmesh_cache_release(golem_full);
    animation_cache_release(golem_animation_set);
}
