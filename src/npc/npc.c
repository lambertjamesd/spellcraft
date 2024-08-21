#include "npc.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../resource/animation_cache.h"
#include "../collision/collision_scene.h"
#include "../cutscene/cutscene_runner.h"

struct npc_information npc_information[] = {
    [NPC_TYPE_MENTOR] = {
        .mesh = "rom:/meshes/characters/mentor.tmesh",
        .animations = "rom:/meshes/characters/mentor.anim",
        .collider = {
            .minkowsi_sum = dynamic_object_capsule_minkowski_sum,
            .bounding_box = dynamic_object_capsule_bounding_box,
            .data = {
                .capsule = {
                    .radius = 0.25f,
                    .inner_half_height = 0.75f,
                },
            },
        },
        .half_height = 1.0f,
    },
};

void npc_interact(struct interactable* interactable, entity_id from) {
    struct npc* npc = (struct npc*)interactable->data;

    if (npc->talk_to_cutscene) {
        cutscene_runner_run(npc->talk_to_cutscene, NULL, NULL);
    }
}

void npc_update(void *data) {
    struct npc* npc = (struct npc*)data;

    animator_update(&npc->animator, npc->renderable.armature.pose, fixed_time_step);
}

void npc_init(struct npc* npc, struct npc_definition* definiton) {
    entity_id entity_id = entity_id_new();

    struct npc_information* information = &npc_information[definiton->npc_type];

    npc->transform.position = definiton->position;
    npc->transform.rotation = definiton->rotation;
    renderable_single_axis_init(&npc->renderable, &npc->transform, information->mesh);

    render_scene_add_renderable_single_axis(&npc->renderable, 2.0f);

    update_add(npc, npc_update, 0, UPDATE_LAYER_WORLD);
    animator_init(&npc->animator, npc->renderable.armature.bone_count);

    npc->animation_set = information->animations ? animation_cache_load(information->animations) : NULL;

    npc->animations.idle = animation_set_find_clip(npc->animation_set, "mentor_idle");
    animator_run_clip(&npc->animator, npc->animations.idle, 0.0f, true);

    dynamic_object_init(
        entity_id,
        &npc->collider,
        &information->collider,
        COLLISION_LAYER_TANGIBLE,
        &npc->transform.position,
        &npc->transform.rotation
    );

    npc->collider.center.y += information->half_height;
    npc->collider.is_fixed = 1;

    collision_scene_add(&npc->collider);

    interactable_init(&npc->interactable, entity_id, npc_interact, npc);

    if (*definiton->dialog) {
        npc->talk_to_cutscene = cutscene_load(definiton->dialog);
    } else {
        npc->talk_to_cutscene = NULL;
    }
}

void npc_destroy(struct npc* npc) {
    renderable_single_axis_destroy(&npc->renderable);
    render_scene_remove(&npc->renderable);
    update_remove(npc);
    collision_scene_remove(&npc->collider);
    animation_cache_release(npc->animation_set);
    interactable_destroy(&npc->interactable);
    cutscene_free(npc->talk_to_cutscene);
}