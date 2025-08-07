#include "treasure_chest.h"

#include "../collision/collision_scene.h"
#include "../collision/shapes/box.h"
#include "../cutscene/cutscene_runner.h"
#include "../cutscene/show_item.h"
#include "../player/inventory.h"
#include "../render/render_scene.h"
#include "../resource/animation_cache.h"
#include "../time/time.h"

static struct dynamic_object_type treasure_chest_collision = {
    BOX_COLLIDER(0.4f, 0.35f, 0.35f),
    .bounce = 0.2f,
    .friction = 0.25f,
};

void treasure_chest_interact(struct interactable* interactable, entity_id from) {
    struct treasure_chest* treasure_chest = (struct treasure_chest*)interactable->data;

    if (!treasure_chest->item_type) {
        return;
    }

    animator_run_clip(&treasure_chest->animator, treasure_chest->animations.open, 0.0f, false);

    struct cutscene_builder builder;
    cutscene_builder_init(&builder);

    cutscene_builder_pause(&builder, true, false, UPDATE_LAYER_WORLD);
    cutscene_builder_delay(&builder, 1.0f);

    show_item_in_cutscene(&builder, treasure_chest->item_type);

    cutscene_runner_run(
        cutscene_builder_finish(&builder),
        cutscene_runner_free_on_finish(),
        NULL
    );

    inventory_unlock_item(treasure_chest->item_type);
    treasure_chest->item_type = ITEM_TYPE_NONE;
}

void treasure_chest_update(void* data) {
    struct treasure_chest* treasure_chest = (struct treasure_chest*)data;
    animator_update(&treasure_chest->animator, &treasure_chest->renderable.armature, fixed_time_step);
}

void treasure_chest_init(struct treasure_chest* treasure_chest, struct treasure_chest_definition* definition, entity_id id) {
    treasure_chest->item_type = definition->item;
    transformSaInit(&treasure_chest->transform, &definition->position, &definition->rotation, 1.0f);

    renderable_single_axis_init(&treasure_chest->renderable, &treasure_chest->transform, "rom:/meshes/objects/treasurechest.tmesh");
    render_scene_add_renderable(&treasure_chest->renderable, 0.8f);

    dynamic_object_init(
        id, 
        &treasure_chest->dynamic_object, 
        &treasure_chest_collision, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE,
        &treasure_chest->transform.position, 
        0
    );

    treasure_chest->dynamic_object.center.y = treasure_chest_collision.data.box.half_size.y;
    treasure_chest->dynamic_object.is_fixed = true;

    collision_scene_add(&treasure_chest->dynamic_object);

    interactable_init(&treasure_chest->interactable, id, treasure_chest_interact, treasure_chest);

    treasure_chest->animation_set = animation_cache_load("rom:/meshes/objects/treasurechest.anim");
    treasure_chest->animations.open = animation_set_find_clip(treasure_chest->animation_set, "open");
    treasure_chest->animations.idle = animation_set_find_clip(treasure_chest->animation_set, "idle");

    animator_init(&treasure_chest->animator, treasure_chest->renderable.armature.bone_count);
    update_add(treasure_chest, treasure_chest_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    if (inventory_has_item(definition->item) && !inventory_is_upgrade_item(definition->item)) {
        animator_run_clip(&treasure_chest->animator, treasure_chest->animations.open, animation_clip_get_duration(treasure_chest->animations.open), false);
        treasure_chest->item_type = ITEM_TYPE_NONE;
    } else {
        animator_run_clip(&treasure_chest->animator, treasure_chest->animations.idle, 0.0f, false);
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