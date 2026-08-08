#include "mesh_animation.h"

#include "../resource/tmesh_cache.h"
#include "../resource/animation_cache.h"
#include "effect_allocator.h"
#include "../render/render_scene.h"
#include "../time/time.h"

struct mesh_animation* mesh_animation_new(struct Vector3* position, struct Vector2* rotation, struct tmesh* mesh, struct animation_clip* clip) {
    struct mesh_animation* result = effect_malloc(sizeof(struct mesh_animation));

    transformSaInit(&result->transform, position, rotation, 1.0f);

    renderable_single_axis_init_direct(&result->renderable, &result->transform, mesh);
    renderable_set_animator(&result->renderable, &result->animtor);

    render_scene_add_renderable(&result->renderable, 1.0f);

    animator_init(&result->animtor, mesh->armature.bone_count);
    animator_run_clip(&result->animtor, clip, 0.0f, false);

    result->animations = NULL;

    return result;
}

bool mesh_animation_update(struct mesh_animation* mesh_animation) {
    animator_update(&mesh_animation->animtor, fixed_time_step);
    return animator_is_running(&mesh_animation->animtor);
}

void mesh_animation_free(struct mesh_animation* mesh_animation) {
    render_scene_remove(&mesh_animation->renderable);
    animator_destroy(&mesh_animation->animtor);
    effect_free(mesh_animation);
}

void mesh_animation_one_shot_update(void* data) {
    struct mesh_animation* mesh_animation = (struct mesh_animation*)data;

    if (!mesh_animation_update(mesh_animation)) {
        tmesh_cache_release(mesh_animation->renderable.mesh_render.mesh);
        if (mesh_animation->animations) {
            animation_cache_release(mesh_animation->animations);
        }
        mesh_animation_free(mesh_animation);
        update_remove(mesh_animation);
    }
    
}

void mesh_animation_one_shot(struct Vector3* position, struct Vector2* rotation, const char* mesh_filename, const char* animation_set, const char* animation) {
    tmesh_t* mesh = tmesh_cache_load(mesh_filename);
    animation_set_t* animations = animation_cache_load(animation_set);
    animation_clip_t* clip = animation_set_find_clip(animations, animation);

    struct mesh_animation* mesh_animation = mesh_animation_new(position, rotation, mesh, clip);
    mesh_animation->animations = animations;
    update_add(mesh_animation, mesh_animation_one_shot_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
}