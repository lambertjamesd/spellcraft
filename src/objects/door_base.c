#include "door_base.h"

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

#define COLLISION_LAYERS    (COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE | COLLISION_LAYER_BLOCK_CAMERA)

static struct dynamic_object_type door_collision = {
    BOX_COLLIDER(2.16264f, 2.0f, 0.317424f),
    .bounce = 0.2f,
    .friction = 0.25f,
    .center = { 0.0f, 2.0f, 0.0f },
};

void door_base_destroy_lock(door_base_t* door) {
    door->interact_blocker = NULL;
    
    if (door->lock_model) {
        tmesh_cache_release(door->lock_model);
        door->lock_model = NULL;
    }
    if (door->lock_animation_set) {
        animation_cache_release(door->lock_animation_set);
        door->lock_animation_set = NULL;
    }
    
    animator_destroy(&door->lock_animator);
    armature_destroy(&door->lock_armatrue);
}

void door_base_render(void* data, struct render_batch* batch) {
    door_base_t* door = (door_base_t*)data;
    
    render_scene_render_renderable_single_axis(&door->renderable, batch);

    if (!door->lock_model) {
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

    animator_apply(&door->lock_animator, &door->lock_armatrue);
    render_batch_add_tmesh(batch, door->lock_model, mtxfp, &door->lock_armatrue, NULL, NULL);
}

void door_cutscene_open(void* data) {
    door_base_t* door = (door_base_t*)data;
    animator_run_clip(&door->animator, door->animations.open, 0.0f, false);
    door->collider.collision_layers = 0;
}

void door_cutscene_close(void* data) {
    door_base_t* door = (door_base_t*)data;
    animator_run_clip(&door->animator, door->animations.close, 0.0f, false);
    door->next_room = door->preview_room;
    door->collider.collision_layers = COLLISION_LAYERS;
}

void door_base_interact(struct interactable* interactable, entity_id from) {
    struct dynamic_object* obj = collision_scene_find_object(from);

    if (!obj) {
        return;
    }

    door_base_t* door = (door_base_t*)interactable->data;

    if (door->is_unlocking) {
        return;
    }
    
    if (door->interact_blocker && !door->interact_blocker(interactable, from)) {
        return;
    }

    room_id other_room = scene_is_showing_room(current_scene, door->room_a) ? door->room_b : door->room_a;
    scene_show_room(current_scene, other_room);
    door->preview_room = other_room;

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

    struct cutscene_builder builder;
    cutscene_builder_init(&builder);

    cutscene_builder_pause(&builder, true, false);

    if (door->is_unlocking && animator_is_running(&door->lock_animator)) {
        cutscene_builder_delay(&builder, animation_clip_get_duration(door->lock_animator.current_clip));
    }

    cutscene_builder_callback(&builder, door_cutscene_open, door);

    cutscene_builder_delay(&builder, 0.75f);
    cutscene_builder_interact_position(
        &builder, 
        INTERACTION_MOVE, 
        ENTITY_ID_PLAYER,
        &target
    );
    cutscene_builder_npc_set_speed(&builder, ENTITY_ID_PLAYER, 2.0f);
    cutscene_builder_delay(&builder, 0.4f);
    cutscene_builder_camera_move_to(
        &builder, 
        &camera_target, 
        true
    );
    target.y += CAMERA_FOLLOW_HEIGHT;
    cutscene_builder_camera_look_at_pos(
        &builder, 
        &target, 
        true
    );
    cutscene_builder_npc_wait(&builder, ENTITY_ID_PLAYER);
    cutscene_builder_callback(&builder, door_cutscene_close, door);
    vector3AddScaled(&door->transform.position, &offset, BEHIND_PLAYER_OFFSET, &camera_target);
    camera_target.y += CAMERA_FOLLOW_HEIGHT;
    cutscene_builder_camera_move_to(
        &builder, 
        &camera_target, 
        false
    );
    cutscene_builder_camera_wait(&builder);
    cutscene_builder_camera_follow(&builder);
    cutscene_builder_pause(&builder, false, false);

    cutscene_runner_run(
        cutscene_builder_finish(&builder),
        0,
        cutscene_runner_free_on_finish(),
        door,
        0
    );
}

void door_base_update(door_base_t* door) {
    animator_update(&door->animator, fixed_time_step);
    animator_update(&door->lock_animator, fixed_time_step);

    if (door->next_room != ROOM_NONE && !animator_is_running(&door->animator)) {
        scene_hide_room(current_scene, door->next_room == door->room_a ? door->room_b : door->room_a);
        door->next_room = ROOM_NONE;
        door->preview_room = ROOM_NONE;
    }

    if (door->is_unlocking && !animator_is_running(&door->lock_animator)) {
        interactable_set_type(&door->interactable, INTERACT_TYPE_OPEN);
        door_base_destroy_lock(door);
        door->is_unlocking = false;
    }
}

void door_base_init(door_base_t* door, door_base_definition_t* definition, entity_id id, const char* mesh_filename) {
    transformSaInit(&door->transform, &definition->position, &definition->rotation, 1.0f);
    door->room_a = definition->room_a;
    door->room_b = definition->room_b;
    door->next_room = ROOM_NONE;
    door->preview_room = ROOM_NONE;

    renderable_single_axis_init(&door->renderable, &door->transform, mesh_filename);
    renderable_set_animator(&door->renderable, &door->animator);
    render_scene_add(&door->transform.position, 2.4f, door_base_render, door);

    dynamic_object_init(
        id, 
        &door->collider, 
        &door_collision, 
        COLLISION_LAYERS,
        &door->transform.position, 
        &door->transform.rotation
    );

    door->collider.is_fixed = true;
    door->collider.weight_class = WEIGHT_CLASS_HEAVY;
    door->lock_model = NULL;
    door->lock_animation_set = NULL;
    door->is_unlocking = false;
    animator_init(&door->lock_animator, 0);
    armature_init(&door->lock_armatrue, NULL);

    collision_scene_add(&door->collider);

    interactable_init(&door->interactable, id, INTERACT_TYPE_OPEN, door_base_interact, door);

    door->animation_set = animation_cache_load("rom:/meshes/objects/doors/door.anim");
    door->animations.open = animation_set_find_clip(door->animation_set, "open");
    door->animations.close = animation_set_find_clip(door->animation_set, "close");

    animator_init(&door->animator, door->renderable.mesh_render.armature.bone_count);
    animator_run_clip(&door->animator, door->animations.close, animation_clip_get_duration(door->animations.close), false);
}

void door_base_destroy(door_base_t* door) {
    renderable_destroy(&door->renderable);
    render_scene_remove(door);
    collision_scene_remove(&door->collider);
    animator_destroy(&door->animator);
    if (door->animation_set) {
        animation_cache_release(door->animation_set);
    }

    door_base_destroy_lock(door);
}

void door_base_lock(door_base_t* door, door_lock_definition_t* lock_definition) {
    assert(door_base_is_unlocked(door));
    
    if (lock_definition->interact_blocker) {
        interactable_set_type(&door->interactable, INTERACT_TYPE_CHECK);
    } else {
        interactable_set_type(&door->interactable, INTERACT_TYPE_NONE);
    }
    
    door->interact_blocker = lock_definition->interact_blocker;

    door->lock_model = tmesh_cache_load(lock_definition->mesh_filename);
    door->animation_set = lock_definition->animations_filename ? animation_cache_load(lock_definition->animations_filename) : NULL;

    animator_init(&door->lock_animator, door->lock_model->armature.bone_count);
    armature_init(&door->lock_armatrue, &door->lock_model->armature);

    animator_run_clip(&door->lock_animator, animation_set_find_clip(door->animation_set, "lock"), 0.0f, false);
    door->is_unlocking = false;
}

void door_base_unlock(door_base_t* door) {
    assert(!door_base_is_unlocked(door));

    if (door->is_unlocking) {
        return;
    }

    door->is_unlocking = true;
    animator_run_clip(&door->lock_animator, animation_set_find_clip(door->animation_set, "unlock"), 0.0f, false);
}

bool door_base_is_unlocked(door_base_t* door) {
    return door->lock_model == NULL;
}