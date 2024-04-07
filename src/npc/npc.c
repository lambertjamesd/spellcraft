#include "npc.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../resource/animation_cache.h"

void npc_update(void *data) {
    struct npc* npc = (struct npc*)data;

    animator_update(&npc->animator, npc->renderable.armature.pose, fixed_time_step);
}

void npc_init(struct npc* npc, struct npc_definition* definiton) {
    npc->transform.position = definiton->position;
    npc->transform.rotation = definiton->rotation;
    renderable_single_axis_init(&npc->renderable, &npc->transform, "rom:/meshes/characters/mentor.mesh");

    render_scene_add_renderable_single_axis(&npc->renderable, 2.0f);

    update_add(npc, npc_update, 0, UPDATE_LAYER_WORLD);
    animator_init(&npc->animator, npc->renderable.armature.bone_count);

    npc->animation_set = animation_cache_load("rom:/meshes/characters/mentor.anim");

    npc->animations.idle = animation_set_find_clip(npc->animation_set, "mentor_idle");
    animator_run_clip(&npc->animator, npc->animations.idle, 0.0f, true);
}

void npc_destroy(struct npc* npc) {
    renderable_single_axis_destroy(&npc->renderable);
    render_scene_remove(&npc->renderable);
    update_remove(npc);
    animation_cache_release(npc->animation_set);
}