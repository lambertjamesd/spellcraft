#include "door.h"

#include "../collision/shapes/box.h"
#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include "../resource/animation_cache.h"
#include "../cutscene/cutscene_runner.h"
#include "../cutscene/expression_evaluate.h"
#include "../scene/scene.h"
#include "../resource/tmesh_cache.h"

#define BEHIND_PLAYER_OFFSET        0.5f
#define DOOR_EXIT_SPACING           1.5f
#define CAMERA_PLACEMENT_OFFSET     3.0f
#define CAMERA_PLACEMENT_TANGENTS   2.0f

static struct dynamic_object_type door_collision = {
    BOX_COLLIDER(1.0f, 1.5f, 0.35f),
    .bounce = 0.2f,
    .friction = 0.25f,
};

void door_render(void* data, struct render_batch* batch) {
    struct door* door = (struct door*)data;
    
    render_scene_render_renderable_single_axis(&door->renderable, batch);

    if (door->is_unlocked) {
        return;
    }

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    transformSAToMatrix(&door->transform, mtx);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(batch, door->lock_model, mtxfp, NULL, NULL, NULL);
}

void door_cuscene_finish(struct cutscene* cutscene, void* data) {
    struct door* door = (struct door*)data;

    animator_run_clip(&door->animator, door->animations.close, 0.0f, false);
    
    door->next_room = door->preview_room;
    door->collider.collision_layers = COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE;

    cutscene_free(cutscene);
}

bool door_interact(struct interactable* interactable, entity_id from) {
    struct dynamic_object* obj = collision_scene_find_object(from);

    if (!obj) {
        return false;
    }

    struct door* door = (struct door*)interactable->data;

    if (!door->is_unlocked) {
        return false;
    }

    room_id other_room = scene_is_showing_room(current_scene, door->room_a) ? door->room_b : door->room_a;
    scene_show_room(current_scene, other_room);
    door->preview_room = other_room;

    struct cutscene_builder builder;
    cutscene_builder_init(&builder);

    struct Vector3 offset;
    vector3Sub(&door->transform.position, obj->position, &offset);
    struct Vector3 door_forward;
    vector2ToLookDir(&door->transform.rotation, &door_forward);
    vector3Project(&offset, &door_forward, &offset);
    vector3Normalize(&offset, &offset);

    struct Vector3 target;
    vector3AddScaled(&door->transform.position, &offset, DOOR_EXIT_SPACING, &target);
    
    struct Vector3 camera_target;
    vector3AddScaled(
        &door->transform.position,
        &offset,
        CAMERA_PLACEMENT_OFFSET,
        &camera_target
    );
    camera_target.y += CAMERA_FOLLOW_HEIGHT;
    struct Vector3 door_right;
    vector3Rotate90(&offset, &door_right);
    vector3AddScaled(&camera_target, &door_right, CAMERA_PLACEMENT_TANGENTS, &camera_target);

    animator_run_clip(&door->animator, door->animations.open, 0.0f, false);

    door->collider.collision_layers = 0;

    cutscene_builder_pause(&builder, true, false, UPDATE_LAYER_WORLD);
    cutscene_builder_delay(&builder, 0.5f);
    cutscene_builder_interact_position(
        &builder, 
        INTERACTION_MOVE, 
        (union cutscene_actor_id) {
            .npc_type = NPC_TYPE_PLAYER,
        },
        &target
    );
    cutscene_builder_delay(&builder, 0.25f);
    cutscene_builder_camera_move_to(
        &builder, 
        &camera_target, 
        &(camera_move_to_args_t){
            .instant = true, 
            .move_target = false
        }
    );
    target.y += CAMERA_FOLLOW_HEIGHT;
    cutscene_builder_camera_move_to(
        &builder, 
        &target, 
        &(camera_move_to_args_t){
            .instant = true, 
            .move_target = true
        }
    );
    cutscene_builder_npc_wait(
        &builder, 
        (union cutscene_actor_id) {
            .npc_type = NPC_TYPE_PLAYER,
        }
    );
    vector3AddScaled(&door->transform.position, &offset, BEHIND_PLAYER_OFFSET, &camera_target);
    camera_target.y += CAMERA_FOLLOW_HEIGHT;
    cutscene_builder_camera_move_to(
        &builder, 
        &camera_target, 
        &(camera_move_to_args_t){
            .instant = false, 
            .move_target = false
        }
    );
    cutscene_builder_camera_wait(&builder);
    cutscene_builder_camera_return(&builder);
    cutscene_builder_pause(&builder, false, false, UPDATE_LAYER_WORLD);

    cutscene_runner_run(
        cutscene_builder_finish(&builder),
        door_cuscene_finish,
        door,
        0
    );

    return true;
}

void door_update(void* data) {
    struct door* door = (struct door*)data;
    animator_update(&door->animator, &door->renderable.armature, fixed_time_step);

    if (door->next_room != ROOM_NONE && !animator_is_running(&door->animator)) {
        scene_hide_room(current_scene, door->next_room == door->room_a ? door->room_b : door->room_a);
        door->next_room = ROOM_NONE;
        door->preview_room = ROOM_NONE;
    }
    
    door->is_unlocked = door->unlocked == VARIABLE_DISCONNECTED ? true : expression_get_bool(door->unlocked);
}

void door_init(struct door* door, struct door_definition* definition, entity_id id) {
    transformSaInit(&door->transform, &definition->position, &definition->rotation, 1.0f);
    door->room_a = definition->room_a;
    door->room_b = definition->room_b;
    door->unlocked = definition->unlocked;
    door->next_room = ROOM_NONE;
    door->preview_room = ROOM_NONE;

    renderable_single_axis_init(&door->renderable, &door->transform, "rom:/meshes/objects/doors/door.tmesh");
    render_scene_add(&door->transform.position, 1.4f, door_render, door);

    dynamic_object_init(
        id, 
        &door->collider, 
        &door_collision, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE,
        &door->transform.position, 
        &door->transform.rotation
    );

    door->collider.center.y = door_collision.data.box.half_size.y;
    door->collider.is_fixed = true;
    door->collider.weight_class = WEIGHT_CLASS_HEAVY;
    door->lock_model = tmesh_cache_load("rom:/meshes/objects/doors/lock.tmesh");

    collision_scene_add(&door->collider);

    interactable_init(&door->interactable, id, door_interact, door);

    door->animation_set = animation_cache_load("rom:/meshes/objects/doors/door.anim");
    door->animations.open = animation_set_find_clip(door->animation_set, "open");
    door->animations.close = animation_set_find_clip(door->animation_set, "close");

    animator_init(&door->animator, door->renderable.armature.bone_count);
    update_add(door, door_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
    animator_run_clip(&door->animator, door->animations.close, animation_clip_get_duration(door->animations.close), false);

    door->is_unlocked = door->unlocked == VARIABLE_DISCONNECTED ? true : expression_get_bool(door->unlocked);
}

void door_destroy(struct door* door) {
    renderable_destroy(&door->renderable);
    render_scene_remove(door);
    collision_scene_remove(&door->collider);
    animator_destroy(&door->animator);
    animation_cache_release(door->animation_set);
    update_remove(door);
    tmesh_cache_release(door->lock_model);
}