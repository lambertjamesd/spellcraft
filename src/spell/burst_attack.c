#include "burst_attack.h"

#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "../resource/animation_cache.h"
#include "../time/time.h"

static burst_attack_definition_t ground_fire = {
    .mesh_name = "rom:/meshes/spell/groundfire.tmesh",
    .animation_file = "rom:/meshes/spell/groundfire.anim",
    .animation = "attack",
};

void burst_attack_init(burst_attack_t* burst_attack, struct spell_data_source* source, struct spell_event_options event_options, burst_attack_definition_t* effect_definition) {
    vector2_t rotation;
    vector2LookDir(&rotation, &source->direction);
    transformSaInit(&burst_attack->transform, &source->position, &rotation, 1.0f);

    renderable_single_axis_init(&burst_attack->renderable, &burst_attack->transform, ground_fire.mesh_name);
    render_scene_add_renderable(&burst_attack->renderable, 0.0f);
    renderable_set_animator(&burst_attack->renderable, &burst_attack->animator);

    burst_attack->animations = animation_cache_load(ground_fire.animation_file);

    animator_init(&burst_attack->animator, burst_attack->renderable.mesh_render.armature.bone_count);
    animator_run_clip(&burst_attack->animator, animation_set_find_clip(burst_attack->animations, ground_fire.animation), 0.0f, false);
}

void burst_attack_destroy(burst_attack_t* burst_attack) {
    render_scene_remove(&burst_attack->renderable);
    renderable_destroy(&burst_attack->renderable);

    animation_cache_release(burst_attack->animations);
}

bool burst_attack_update(burst_attack_t* burst_attack) {
    animator_update(&burst_attack->animator, fixed_time_step);

    return animator_is_running(&burst_attack->animator);
}