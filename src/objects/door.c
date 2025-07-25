#include "door.h"

#include "../collision/shapes/box.h"
#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include "../resource/animation_cache.h"
#include "../cutscene/cutscene_runner.h"
#include "../scene/scene.h"

#define DOOR_EXIT_SPACING   1.0f

static struct dynamic_object_type door_collision = {
    BOX_COLLIDER(1.0f, 1.5f, 0.35f),
    .bounce = 0.2f,
    .friction = 0.25f,
};

void door_cuscene_finish(struct cutscene* cutscene, void* data) {
    struct door* door = (struct door*)data;

    animator_run_clip(&door->animator, door->animations.close, 0.0f, false);
    
    door->next_room = current_scene->current_room == door->room_a ? door->room_b : door->room_a;
    door->collider.collision_layers = COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE;

    cutscene_free(cutscene);
}

void door_interact(struct interactable* interactable, entity_id from) {
    struct dynamic_object* obj = collision_scene_find_object(from);

    if (!obj) {
        return;
    }

    struct door* door = (struct door*)interactable->data;

    room_id other_room = current_scene->current_room == door->room_a ? door->room_b : door->room_a;
    current_scene->preview_room = other_room;

    struct cutscene_builder builder;
    cutscene_builder_init(&builder);

    struct Vector3 offset;
    vector3Sub(&door->transform.position, obj->position, &offset);
    struct Vector3 door_forward;
    vector2ToLookDir(&door->transform.rotation, &door_forward);
    vector3Project(&offset, &door_forward, &offset);
    vector3Normalize(&offset, &offset);
    vector3Scale(&offset, &offset, DOOR_EXIT_SPACING);

    struct Vector3 target;
    vector3Add(&door->transform.position, &offset, &target);

    animator_run_clip(&door->animator, door->animations.open, 0.0f, false);

    door->collider.collision_layers = 0;

    cutscene_builder_pause(&builder, true, false, UPDATE_LAYER_WORLD);
    cutscene_builder_delay(&builder, 0.5f);
    cutscene_builder_interact_position(
        &builder, 
        INTERACTION_MOVE_WAIT, 
        (union cutscene_actor_id) {
            .npc_type = NPC_TYPE_PLAYER,
        },
        &target
    );
    cutscene_builder_pause(&builder, false, false, UPDATE_LAYER_WORLD);

    cutscene_runner_run(
        cutscene_builder_finish(&builder),
        door_cuscene_finish,
        door
    );
}

void door_update(void* data) {
    struct door* door = (struct door*)data;
    animator_update(&door->animator, door->renderable.armature.pose, fixed_time_step);

    if (door->next_room != ROOM_NONE && !animator_is_running(&door->animator)) {
        current_scene->current_room = door->next_room;
        current_scene->preview_room = ROOM_NONE;
        door->next_room = ROOM_NONE;
    }
}

void door_init(struct door* door, struct door_definition* definition) {
    door->transform.position = definition->position;
    door->transform.rotation = definition->rotation;
    door->room_a = definition->room_a;
    door->room_b = definition->room_b;
    door->next_room = ROOM_NONE;

    renderable_single_axis_init(&door->renderable, &door->transform, "rom:/meshes/objects/doors/door.tmesh");
    render_scene_add_renderable(&door->renderable, 0.8f);

    entity_id entity_id = entity_id_new();

    dynamic_object_init(
        entity_id, 
        &door->collider, 
        &door_collision, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE,
        &door->transform.position, 
        0
    );

    door->collider.center.y = door_collision.data.box.half_size.y;
    door->collider.is_fixed = true;

    collision_scene_add(&door->collider);

    interactable_init(&door->interactable, entity_id, door_interact, door);

    door->animation_set = animation_cache_load("rom:/meshes/objects/doors/door.anim");
    door->animations.open = animation_set_find_clip(door->animation_set, "open");
    door->animations.close = animation_set_find_clip(door->animation_set, "close");

    animator_init(&door->animator, door->renderable.armature.bone_count);
    update_add(door, door_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
}

void door_destroy(struct door* door) {
    renderable_destroy(&door->renderable);
    render_scene_remove(&door->renderable);
    collision_scene_remove(&door->collider);
    animator_destroy(&door->animator);
    animation_cache_release(door->animation_set);
    update_remove(door);
}