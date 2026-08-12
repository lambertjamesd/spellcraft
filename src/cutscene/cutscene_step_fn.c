#include "cutscene_step_fn.h"
#include "../menu/hud.h"
#include "../scene/scene.h"
#include "../menu/dialog_box.h"
#include "../time/time.h"
#include "../time/game_mode.h"
#include "../objects/empty.h"
#include "../effects/image_overlay.h"
#include "../effects/area_title.h"
#include "../effects/fade_effect.h"
#include "../entities/comm_stone.h"
#include "../render/fog.h"
#include "cutscene_stopwatch.h"
#include "cutscene_timer.h"
#include "show_item.h"

#define READ_ARGS(context, expected, argc, args)    \
        assert(expected == argc); \
        int args[expected]; \
        evaluation_context_popn(&context->eval, args, expected);

static color_t fade_colors[] = {
    {0, 0, 0, 0},
    {0, 0, 0, 255},
    {255, 255, 255, 255},
};

cutscene_actor_t* cutscene_lookup_actor(cutscene_runner_context_t* context, entity_id input) {
    return cutscene_actor_find(cutscene_context_get_translate_entity(context, input));
}

vector3_t* cutscene_lookup_pos(cutscene_runner_context_t* context, entity_id id) {
    id = cutscene_context_get_translate_entity(context, id);

    cutscene_actor_t* actor = cutscene_actor_find(id);

    if (actor != NULL) {
        return cutscene_actor_get_pos(actor);
    }

    empty_t* empty = empty_find(id);

    if (empty != NULL) {
        return &empty->position;
    }

    dynamic_object_t* obj = collision_scene_find_object(id);

    if (obj) {
        return obj->position;
    }

    return NULL;
}

// say/ask
void cutscene_say_ask_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 1);
}

bool cutscene_say_step(cutscene_runner_context_t* context) {
    int has_shown = context->stack_depth == evaluation_context_stack_size(&context->eval);

    if (!has_shown && !dialog_box_is_active()) {
        char* message = (char*)evaluation_context_pop(&context->eval);
        dialog_box_show(message, NULL, NULL, NULL);
    } else if (has_shown) {
        return !dialog_box_is_active();
    }

    return false;
}

void cutscene_say_ask_cancel(cutscene_runner_context_t* context) {
    dialog_box_hide();
}

bool cutscene_ask_step(cutscene_runner_context_t* context) {
    int has_shown = context->stack_depth == evaluation_context_stack_size(&context->eval);

    if (!has_shown && !dialog_box_is_active()) {
        char* message = (char*)evaluation_context_pop(&context->eval);
        dialog_box_ask(message, NULL, NULL, NULL);
    } else if (has_shown && !dialog_box_is_active()) {
        evaluation_context_push(&context->eval, dialog_box_get_response());
        return true;
    }

    return false;
}

// show_item

void cutscene_show_item_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 2, arg_count, args);
    show_item_start(args[0], args[1]);
}

bool cutscene_show_item_step(cutscene_runner_context_t* context) {
    return show_item_update();
}

void cutscene_show_item_cancel(cutscene_runner_context_t* context) {
    show_item_cleanup();
}

// pause

void cutscene_step_pause_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 0);
    update_pause_layers(UPDATE_LAYER_WORLD);
}

void cutscene_step_unpause_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 0);
    update_unpause_layers(UPDATE_LAYER_WORLD);
}

// enter_menu

void cutscene_step_enter_menu(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 0);
    update_pause_layers(UPDATE_LAYER_WORLD);
    current_game_mode = GAME_MODE_TRANSITION_TO_MENU;
}

void cutscene_step_exit_menu(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 0);
    update_unpause_layers(UPDATE_LAYER_WORLD);
    current_game_mode = GAME_MODE_3D;
}

// delay

void cutscene_step_delay_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 1);
}

bool cutscene_step_delay_step(cutscene_runner_context_t* context) {
    float time = evaluation_context_pop_float(&context->eval);
    time -= fixed_time_step;

    if (time <= 0.0f) {
        return true;
    }

    evaluation_context_push_float(&context->eval, time);
    return false;
}

// interact_with_npc
void cutscene_interact_with_npc_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 3, arg_count, args);

    struct cutscene_actor* subject = cutscene_lookup_actor(context, args[1]);
    if (!subject) {
        return;
    }

    vector3_t* target = cutscene_lookup_pos(context, args[2]);
    if (!target) {
        return;
    }

    cutscene_actor_interact_with(
        subject,
        args[0],
        target
    );
}

// idle_npc
void cutscene_idle_npc_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 1, arg_count, args);
    struct cutscene_actor* subject = cutscene_lookup_actor(context, args[0]);
    if (!subject) {
        return;
    }
    cutscene_actor_idle(subject);
}

// cam_look_npc
void cutscene_camera_look_at_npc_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 1, arg_count, args);
    entity_id entity_id = args[0];

    struct cutscene_actor* target = cutscene_lookup_actor(context, entity_id);
    if (target) {
        camera_look_at(&current_scene->camera_controller, &target->transform.position);
        return;
    }

    dynamic_object_t* obj = collision_scene_find_object(entity_id);

    if (obj) {
        vector3_t center;
        vector3Lerp(&obj->bounding_box.min, &obj->bounding_box.max, 0.5f, &center);
        camera_look_at(&current_scene->camera_controller, &center);
        return;
    }

    empty_t* empty = empty_find(entity_id);

    if (empty) {
        camera_look_at(&current_scene->camera_controller, &empty->position);
        return;
    }
}

// cam_follow_player
void cutscene_cam_follow_player_init(cutscene_runner_context_t* context, int arg_count) {
    camera_follow_player(&current_scene->camera_controller);
}

// cam_return
void cutscene_cam_return_init(cutscene_runner_context_t* context, int arg_count) {
    camera_return(&current_scene->camera_controller);
}

// cam_animate
void cutscene_cam_anim_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 1, arg_count, args);
    camera_play_animation(
        &current_scene->camera_controller, 
        camera_animation_lookup(&current_scene->camera_animations, (char*)args[0])
    );
}

// cam_move_to
void cutscene_cam_move_to_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 4, arg_count, args);
    camera_move_to(
        &current_scene->camera_controller, 
        (vector3_t*)args, 
        args[3], 
        false
    );
}

// cam_look_at
void cutscene_cam_look_at_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 4, arg_count, args);
    camera_move_to(
        &current_scene->camera_controller, 
        (vector3_t*)args, 
        args[3], 
        true
    );
}

// camera_wait
void cutscene_camera_wait_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 0);
}

bool cutscene_camera_wait_step(cutscene_runner_context_t* context) {
    return !camera_is_animating(&current_scene->camera_controller);
}

// interact_with_location
void cutscene_interact_with_location_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 3, arg_count, args);

    struct cutscene_actor* subject = cutscene_lookup_actor(context, args[1]);
    if (!subject) {
        return;
    }

    struct named_location* target = scene_find_location((char*)args[2]);
    if (!target) {
        return;
    }

    cutscene_actor_interact_with(
        subject,
        args[0],
        &target->position
    );
}

// fade
void cutscene_fade_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 2, arg_count, args);
    fade_effect_set(fade_colors[args[0]], ((float*)args)[1]);
}

// interact_with_position
void cutscene_interact_with_position_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 5, arg_count, args);
    struct cutscene_actor* subject = cutscene_lookup_actor(context, args[1]);
    if (!subject) {
        return;
    }

    cutscene_actor_interact_with(
        subject,
        args[0],
        (vector3_t*)&args[2]
    );
}

// comm_stone

static entity_id active_comm_stone = 0;
static vector2_t rotate_amount = {
    0.5f, 0.86f,
};
static fog_state_t comm_stone_fog = {
    .color = {30, 10, 70, 255}, 
    .min = 5.0f, 
    .max = -2.0f,
};

#define COMM_END_HEIGHT         1.2f
#define COMM_POS_OFFSET         1.5f
#define CAMERA_DISTANCE         2.0f
#define CAMERA_HEIGHT_OFFSET    -0.1f

void cutscene_comm_stone_end_pos(vector3_t* result) {
    struct comm_stone_definition stone_def;
    
    vector2_t* rot = player_get_rotation(&current_scene->player);

    vector2ToLookDir(rot, result);
    vector3Scale(result, result, COMM_POS_OFFSET);
    vector3Add(player_get_position(&current_scene->player), result, result);

    result->y += COMM_END_HEIGHT;
}

void cutscene_comm_stone_spawn() {
    vector3_t target;
    cutscene_comm_stone_end_pos(&target);

    player_t* player = &current_scene->player;

    vector3_t hand_position;
    armature_transform_position(player->cutscene_actor.armature, 15, &gZeroVec, &hand_position);
    transformSaTransformPoint(&player->cutscene_actor.transform, &hand_position, &hand_position);

    struct comm_stone_definition stone_def;
    stone_def.position = hand_position;
    active_comm_stone = entity_spawn(ENTITY_TYPE_comm_stone, &stone_def);
    comm_stone_t* stone = entity_get(active_comm_stone);

    if (!stone) {
        return;
    }

    comm_stone_activate(stone, &hand_position, &target);
}


void cutscene_comm_stone_start_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 0);
    
    // just in case
    if (active_comm_stone) {
        entity_despawn(active_comm_stone);
        active_comm_stone = 0;
    }
    
    vector3_t target;
    cutscene_comm_stone_end_pos(&target);

    vector3_t* player_pos = player_get_position(&current_scene->player);

    vector3_t camera_look;
    camera_look.x = (target.x + player_pos->x) * 0.5f;
    camera_look.y = target.y;
    camera_look.z = (target.z + player_pos->z) * 0.5f;
    camera_move_to(&current_scene->camera_controller, &camera_look, false, true);

    vector2_t* rot = player_get_rotation(&current_scene->player);
    vector3_t offset;
    vector2ToLookDir(rot, &offset);

    vector3_t camera_pos;
    vector3RotateWith2(&offset, &rotate_amount, &camera_pos);
    vector3AddScaled(&camera_look, &camera_pos, CAMERA_DISTANCE, &camera_pos);
    camera_move_to(&current_scene->camera_controller, &camera_pos, false, false);

    cutscene_actor_run_animation(&current_scene->player.cutscene_actor, "comm_stone_start", false);
}

bool cutscene_comm_stone_start_step(cutscene_runner_context_t* context) { 
    if (active_comm_stone == 0) {
        if (cutscene_actor_is_animating(&current_scene->player.cutscene_actor)) {
            return false;
        }

        cutscene_comm_stone_spawn();
        fog_set(FOG_PRIORITY_EFFECT, comm_stone_fog, 2.0f);
        cutscene_actor_run_animation(&current_scene->player.cutscene_actor, "comm_stone_idle", true);
    }
    
    comm_stone_t* stone = entity_get(active_comm_stone);

    if (!stone) {
        return true;
    }

    return !comm_stone_is_animating(stone);
}

void cutscene_comm_stone_end_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 0);

    comm_stone_t* stone = entity_get(active_comm_stone);

    if (stone) {
        comm_stone_deactivate(stone);
        player_t* player = &current_scene->player;
        armature_transform_position(player->cutscene_actor.armature, 4, &gZeroVec, &stone->from);
        transformSaTransformPoint(&player->cutscene_actor.transform, &stone->from, &stone->from);
    }

    cutscene_actor_run_animation(&current_scene->player.cutscene_actor, "comm_stone_end", false);
    camera_return(&current_scene->camera_controller);
    fog_clear(FOG_PRIORITY_EFFECT, 2.0f);
}

bool cutscene_comm_stone_end_step(cutscene_runner_context_t* context) { 
    comm_stone_t* stone = entity_get(active_comm_stone);

    if (!stone) {
        return true;
    }

    if (comm_stone_is_animating(stone)) {
        return false;
    }

    entity_despawn(active_comm_stone);
    active_comm_stone = 0;
    return true;
}

void cutscene_comm_cancel(cutscene_runner_context_t* context) {
    entity_despawn(active_comm_stone);
    active_comm_stone = 0;
}

// npc_wait
void cutscene_npc_wait_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 1);
}

bool cutscene_npc_wait_step(cutscene_runner_context_t* context) {
    entity_id target = evaluation_context_pop(&context->eval);

    struct cutscene_actor* subject = cutscene_lookup_actor(context, target);
    if (!subject || !cutscene_actor_is_moving(subject)) {
        return true;
    }

    evaluation_context_push(&context->eval, target);
    return false;
}

// set_npc_speed
void cutscene_set_npc_speed_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 2, arg_count, args);

    struct cutscene_actor* subject = cutscene_lookup_actor(context, args[0]);
    if (!subject) {
        return;
    }
    cutscene_actor_set_speed(subject, ((float*)args)[1]);
}

// show_title
void cutscene_show_title_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 1, arg_count, args);
    area_title_show((char*)args[0]);
}

// look_at_subject
void cutscene_look_at_subject_init(cutscene_runner_context_t* context, int arg_count) {
    struct dynamic_object* subject = collision_scene_find_object(context->subject_id);
    if (!subject) {
        return;
    }
    cutscene_actor_interact_with(&current_scene->player.cutscene_actor, INTERACTION_LOOK_SPACE, subject->position);
}

// npc_animate
void cutscene_npc_animate_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 3, arg_count, args);
    entity_id subject_id = args[0];
    cutscene_actor_t* subject = cutscene_lookup_actor(context, subject_id);
    if (!subject) {
        return;
    }
    cutscene_actor_run_animation(subject, (char*)args[1], args[2]);
}

// print
void cutscene_print_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 1, arg_count, args);
    debugf("%s\n", (char*)args[0]);
}

// spawn
void cutscene_spawn_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 1, arg_count, args);
    uint32_t spawner = args[0];
    scene_spawn_entity(current_scene, spawner);
}

// despawn 
void cutscene_despawn_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 1, arg_count, args);
    entity_despawn(args[0]);
}

// load_scene
void cutscene_load_scene_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 1, arg_count, args);
    scene_queue_next((char*)args[0]);
}

// start_timer
void cutscene_start_timer_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 2, arg_count, args);
    cutscene_timer_start(((float*)args)[0], (char*)args[1]);
}

// cancel_timer
void cutscene_cancel_timer_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count = 0);
    cutscene_timer_cancel();
}

// show_stopwatch
void cutscene_show_stopwatch_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 1, arg_count, args);
    cutscene_stopwatch_set_active(args[0]);
}

// run_stopwatch
void cutscene_run_stopwatch_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 1, arg_count, args);
    cutscene_stopwatch_set_running(args[0]);
}

// set_audio_pause
void cutscene_set_audio_pause_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 1, arg_count, args);
    if (args[0]) {
        audio_pause_all();
    } else {
        audio_unpause_all();
    }
}

// show_image
void cutscene_show_image_init(cutscene_runner_context_t* context, int arg_count) {
    READ_ARGS(context, 1, arg_count, args);
    char* filename = (char*)args[0];
    if (!filename || !*filename) {
        image_overlay_set(NULL);
    } else {
        image_overlay_set(filename);
    }
}

// show_boss_health
void cutscene_show_boss_health_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 2);
    int args[2];
    evaluation_context_popn(&context->eval, args, 2);

    hud_show_boss_health(&current_scene->hud, (char*)args[0], cutscene_context_get_translate_entity(context, args[1]));
}

static cutscene_step_fn_t function_steps[] = {
    [CUTSCENE_FN_SAY] = {.init = cutscene_say_ask_init, .step = cutscene_say_step, .cancel = cutscene_say_ask_cancel}, // func say(message: str)
    [CUTSCENE_FN_ASK] = {.init = cutscene_say_ask_init, .step = cutscene_ask_step, .cancel = cutscene_say_ask_cancel}, // func ask(message: str): bool
    [CUTSCENE_FN_PAUSE] = {.init = cutscene_step_pause_init }, // func pause()
    [CUTSCENE_FN_UNPAUSE] = {.init = cutscene_step_unpause_init }, // func unpause()
    [CUTSCENE_FN_ENTER_MENU] = { .init = cutscene_step_enter_menu }, // func enter_menu()
    [CUTSCENE_FN_EXIT_MENU] = { .init = cutscene_step_exit_menu }, // func exit_menu()
    [CUTSCENE_FN_SHOW_ITEM] = {.init = cutscene_show_item_init, .step = cutscene_show_item_step, .cancel = cutscene_show_item_cancel }, // func show_item(item: i32, show: bool)
    [CUTSCENE_FN_DELAY] = {.init = cutscene_step_delay_init, .step = cutscene_step_delay_step }, // func delay(duration: float)
    [CUTSCENE_FN_INTERACT_NPC] = {.init = cutscene_interact_with_npc_init }, // func interact_with_npc(interaction: i32, npc: entity_id, target: entity_id)
    [CUTSCENE_FN_NPC_SET_SPEED] = {.init = cutscene_set_npc_speed_init }, // func npc_set_speed(npc: entity_id, speed: float)
    [CUTSCENE_FN_INTERACT_POSITION] = {.init = cutscene_interact_with_position_init }, // func interact_with_position(interaction: i32, npc: entity_id, x: float, y: float, z: float)
    [CUTSCENE_FN_NPC_WAIT] = {.init = cutscene_npc_wait_init, .step = cutscene_npc_wait_step }, // func npc_wait(npc: entity_id)
    [CUTSCENE_FN_CAMERA_WAIT] = {.init = cutscene_camera_wait_init, .step = cutscene_camera_wait_step }, // func cam_wait()
    [CUTSCENE_FN_CAMERA_FOLLOW] = {.init = cutscene_cam_follow_player_init }, // func cam_follow()
    [CUTSCENE_FN_CAMERA_RETURN] = {.init = cutscene_cam_return_init }, // func cam_return()
    [CUTSCENE_FN_CAMERA_LOOK_AT_NPC] = {.init = cutscene_camera_look_at_npc_init }, // func cam_look_npc(target: entity_id)
    [CUTSCENE_FN_CAMERA_MOVE_TO] = {.init = cutscene_cam_move_to_init }, // func cam_move_to_pos(x: float, y: float, z: float, instant: bool)
    [CUTSCENE_FN_CAMERA_LOOK_AT_POS] = {.init = cutscene_cam_look_at_init }, // func cam_look_at_pos(x: float, y: float, z: float, instant: bool)
    [CUTSCENE_FN_LOAD_SCENE] = {.init = cutscene_load_scene_init }, // func load_scene(scene_name: str)
    [CUTSCENE_FN_LOAD_FADE] = {.init = cutscene_fade_init }, // func fade(fade_to: i32, duration: float)
    [CUTSCENE_FN_COMM_STONE_START] = {.init = cutscene_comm_stone_start_init, .step = cutscene_comm_stone_start_step}, // func comm_stone_start()
    [CUTSCENE_FN_COMM_STONE_END] = {.init = cutscene_comm_stone_end_init, .step = cutscene_comm_stone_end_step, .cancel = cutscene_comm_cancel}, // func comm_stone_end()
    {.init = cutscene_idle_npc_init }, // func idle_npc(npc: entity_id)
    {.init = cutscene_cam_anim_init }, // func cam_animate(animation: str)
    {.init = cutscene_interact_with_location_init }, // func interact_with_location(interaction: i32, npc: entity_id, name: str)
    {.init = cutscene_show_title_init }, // func show_title(title: str)
    {.init = cutscene_look_at_subject_init }, // func look_at_subject()
    {.init = cutscene_npc_animate_init }, // func npc_animate(npc: entity_id, animation: str, loop: bool)
    {.init = cutscene_print_init }, // func print(title: str)
    {.init = cutscene_spawn_init }, // func spawn(spawner: entity_spawner)
    {.init = cutscene_despawn_init }, // func despawn(entity: entity_id)
    {.init = cutscene_show_boss_health_init}, // func show_boss_health(name: str, boss_entity: entity_id)
    {.init = cutscene_start_timer_init }, // func start_timer(time: float, scene_name: str)
    {.init = cutscene_cancel_timer_init }, // func cancel_timer()
    {.init = cutscene_show_stopwatch_init }, // func set_stopwatch_show(value: bool)
    {.init = cutscene_run_stopwatch_init }, // func set_stopwatch_run(value: bool)
    {.init = cutscene_set_audio_pause_init }, // func set_audio_paused(value: bool)
    {.init = cutscene_show_image_init }, // func show_image(filename: str)
};

cutscene_step_fn_t* cutscene_step_lookup_fn(int type) {
    assert(type >= 0 && type < sizeof(function_steps) / sizeof(*function_steps));
    return &function_steps[type];
}