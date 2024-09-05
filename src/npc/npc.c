#include "npc.h"

#include "../collision/collision_scene.h"
#include "../collision/shapes/capsule.h"
#include "../cutscene/cutscene_runner.h"
#include "../render/render_scene.h"
#include "../resource/animation_cache.h"
#include "../time/time.h"

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
        .actor = {
            .eye_level = 1.81147f,
            .move_speed = 1.0f,
            .rotate_speed = 2.0f,
        },
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

    cutscene_actor_update(&npc->cutscene_actor);
}

void npc_init(struct npc* npc, struct npc_definition* definiton) {
    entity_id entity_id = entity_id_new();

    struct npc_information* information = &npc_information[definiton->npc_type];

    npc->transform.position = definiton->position;
    npc->transform.rotation = definiton->rotation;
    renderable_single_axis_init(&npc->renderable, &npc->transform, information->mesh);

    render_scene_add_renderable(&npc->renderable, 2.0f);

    update_add(npc, npc_update, 0, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    struct transform_mixed transform;
    transform_mixed_init_sa(&transform, &npc->transform);

    cutscene_actor_init(
        &npc->cutscene_actor, 
        &information->actor,
        transform, 
        definiton->npc_type, 
        0, 
        &npc->renderable.armature, 
        information->animations
    );

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
    render_scene_remove(&npc->renderable);
    renderable_destroy(&npc->renderable);
    cutscene_actor_destroy(&npc->cutscene_actor);
    update_remove(npc);
    collision_scene_remove(&npc->collider);
    interactable_destroy(&npc->interactable);
    cutscene_free(npc->talk_to_cutscene);
}