#include "treasure_chest.h"

#include "../collision/collision_scene.h"
#include "../collision/shapes/box.h"
#include "../cutscene/cutscene_runner.h"
#include "../cutscene/show_item.h"
#include "../player/inventory.h"
#include "../render/render_scene.h"
#include "../resource/animation_cache.h"
#include "../time/time.h"
#include "../cutscene/expression_evaluate.h"

static struct dynamic_object_type treasure_chest_collision = {
    BOX_COLLIDER(0.4f, 0.35f, 0.35f),
    .bounce = 0.2f,
    .friction = 0.25f,
    .center = { 0.0f, 0.35f, 0.0f },
};

void treasure_chest_interact(struct interactable* interactable, entity_id from) {
    struct treasure_chest* treasure_chest = (struct treasure_chest*)interactable->data;

    animator_run_clip(&treasure_chest->animator, treasure_chest->animations.open, 0.0f, false);

    struct cutscene_builder builder;
    cutscene_builder_init(&builder);

    cutscene_builder_pause(&builder, true, false);
    cutscene_builder_delay(&builder, 1.0f);

    show_item_with_var_in_cutscene(&builder, treasure_chest->item_ref);

    cutscene_runner_run(
        cutscene_builder_finish(&builder),
        0,
        cutscene_runner_free_on_finish(),
        NULL,
        treasure_chest->dynamic_object.entity_id
    );

    expression_set_integer(treasure_chest->item_ref, expression_get_integer(treasure_chest->item_ref) + 1);
    expression_set_bool(treasure_chest->has_item, true);
    treasure_chest->item_ref = VARIABLE_DISCONNECTED;
    interactable_set_type(interactable, INTERACT_TYPE_NONE);
}

void treasure_chest_update(void* data) {
    struct treasure_chest* treasure_chest = (struct treasure_chest*)data;
    animator_update(&treasure_chest->animator, fixed_time_step);
}

void treasure_chest_init(struct treasure_chest* treasure_chest, struct treasure_chest_definition* definition, entity_id id) {
    treasure_chest->item_ref = definition->item;
    treasure_chest->has_item = definition->has_item;
    transformSaInit(&treasure_chest->transform, &definition->position, &definition->rotation, 1.0f);

    renderable_single_axis_init(&treasure_chest->renderable, &treasure_chest->transform, "rom:/meshes/objects/treasurechest.tmesh");
    renderable_set_animator(&treasure_chest->renderable, &treasure_chest->animator);
    render_scene_add_renderable(&treasure_chest->renderable, 0.8f);

    dynamic_object_init(
        id, 
        &treasure_chest->dynamic_object, 
        &treasure_chest_collision, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE,
        &treasure_chest->transform.position, 
        0
    );

    treasure_chest->dynamic_object.is_fixed = true;
    treasure_chest->dynamic_object.weight_class = WEIGHT_CLASS_HEAVY;

    collision_scene_add(&treasure_chest->dynamic_object);

    treasure_chest->animation_set = animation_cache_load("rom:/meshes/objects/treasurechest.anim");
    treasure_chest->animations.open = animation_set_find_clip(treasure_chest->animation_set, "open");
    treasure_chest->animations.idle = animation_set_find_clip(treasure_chest->animation_set, "idle");

    animator_init(&treasure_chest->animator, treasure_chest->renderable.mesh_render.armature.bone_count);
    update_add(treasure_chest, treasure_chest_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    bool has_item;

    if (definition->has_item != VARIABLE_DISCONNECTED) {
        has_item = expression_get_bool(definition->has_item);
    } else if (definition->item != VARIABLE_DISCONNECTED) {
        has_item = expression_get_integer(definition->item) != 0;
    } else {
        has_item = false;
    }

    if (has_item) {
        animator_run_clip(&treasure_chest->animator, treasure_chest->animations.open, animation_clip_get_duration(treasure_chest->animations.open), false);
        treasure_chest->item_ref = VARIABLE_DISCONNECTED;
        interactable_init(&treasure_chest->interactable, id, INTERACT_TYPE_NONE, treasure_chest_interact, treasure_chest);
    } else {
        animator_run_clip(&treasure_chest->animator, treasure_chest->animations.idle, 0.0f, false);
        interactable_init(&treasure_chest->interactable, id, INTERACT_TYPE_OPEN, treasure_chest_interact, treasure_chest);
    }
}

void treasure_chest_destroy(struct treasure_chest* treasure_chest) {
    renderable_destroy(&treasure_chest->renderable);
    render_scene_remove(&treasure_chest->renderable);
    collision_scene_remove(&treasure_chest->dynamic_object);
    animator_destroy(&treasure_chest->animator);
    animation_cache_release(treasure_chest->animation_set);
    update_remove(treasure_chest);
}

void treasure_chest_common_init() {

}

void treasure_chest_common_destroy() {

}