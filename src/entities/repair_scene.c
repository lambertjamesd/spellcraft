#include "repair_scene.h"    

#include "../scene/scene.h"
#include "../render/defs.h"
#include "../util/input.h"
#include "repair_part.h"
#include "../render/screen_coords.h"

static material_t cursor_material;

#define CURSOR_SPEED    300.0f
#define OBJECT_SPEED    5.0f

#define CURSOR_SIZE 32

static color_t basic_color = {255, 255, 255, 255};
static color_t hover_color = {20, 200, 255, 255};

extern void* zbuffer_data;

void repair_scene_render_callback(void* data, struct render_batch* batch) {
    repair_scene_t* scene = (repair_scene_t*)data;
    int x = (int)scene->screen_cursor.x;
    int y = (int)scene->screen_cursor.y;

    int offset = (y * SCREEN_WD + x) * sizeof(uint16_t);
    void* repair_part_raycast_position = (uint8_t*)zbuffer_data + offset;
    
    screen_raycast_check_before(repair_part_raycast_position);

    if (scene->can_drop && scene->grabbed_part) {
        repair_part_t* grabbed_part = repair_part_get(scene->grabbed_part);

        if (grabbed_part) {
            repair_part_render_drop_location(grabbed_part, batch);
        }
    }
}

void repair_scene_render_cursor(repair_scene_t* scene) {
    if (!scene->grabbed_part) {
        material_apply(&cursor_material);
    
        int x = (int)scene->screen_cursor.x - CURSOR_SIZE/2;
        int y = (int)scene->screen_cursor.y - CURSOR_SIZE/2;
    
        rdpq_set_prim_color(scene->hovered_part ? hover_color : basic_color);
        rdpq_texture_rectangle(TILE0, x, y, x + CURSOR_SIZE, y + CURSOR_SIZE, 0, 0);
    }
}

static quaternion_t relative_rotations[4] = {
    {-SQRT_1_2_F, 0.0f, 0.0f, SQRT_1_2_F},
    {0.0f, 0.0f, -SQRT_1_2_F, SQRT_1_2_F},
    {SQRT_1_2_F, 0.0f, 0.0f, SQRT_1_2_F},
    {0.0f, 0.0f, SQRT_1_2_F, SQRT_1_2_F},
};

void repair_scene_handle_grabbed_part(repair_scene_t* scene, joypad_inputs_t input, joypad_buttons_t pressed) {
    int rotation_index = -1;

    if (pressed.c_up) {
        rotation_index = 0;
    } else if (pressed.c_right) {
        rotation_index = 1;
    } else if (pressed.c_down) {
        rotation_index = 2;
    } else if (pressed.c_left) {
        rotation_index = 3;
    }

    repair_part_t* grabbed_part = repair_part_get(scene->grabbed_part);

    if (!grabbed_part) {
        return;
    }

    if (rotation_index != -1 && !grabbed_part->prevent_rotation) {
        quaternion_t rot;
        quatMultiply(&relative_rotations[rotation_index], &grabbed_part->target_rotation, &rot);
        grabbed_part->target_rotation = rot;
    }

    vector3_t up;
    vector3_t right;
    quatMultVector(&scene->camera.transform.rotation, &gUp, &up);
    quatMultVector(&scene->camera.transform.rotation, &gRight, &right);

    vector3AddScaled(
        &grabbed_part->transform.position, 
        &right,
        input_handle_deadzone(input.stick_x) * OBJECT_SPEED * (1.0f / 80.0f) * fixed_time_step,
        &grabbed_part->transform.position
    );
    vector3AddScaled(
        &grabbed_part->transform.position, 
        &up,
        input_handle_deadzone(input.stick_y) * OBJECT_SPEED * (1.0f / 80.0f) * fixed_time_step,
        &grabbed_part->transform.position
    );
}

#define DROP_TOLERNACE  1.0f

bool repair_scene_is_in_right_spot(repair_scene_t* scene, repair_part_t* grabbed_part) {
    if (!grabbed_part) {
        return false;
    }

    screen_coords_from_position(&scene->camera.transform, scene->camera.fov, &grabbed_part->transform.position, &scene->screen_cursor);

    ray_t ray_check;
    ray_check.origin = scene->camera.transform.position;
    vector3Sub(&grabbed_part->transform.position, &ray_check.origin, &ray_check.dir);
    vector3Normalize(&ray_check.dir, &ray_check.dir);

    return repair_part_is_in_right_spot(grabbed_part, &ray_check) && repair_part_can_connect(grabbed_part);
}

bool repair_scene_check_drop(repair_scene_t* scene) {
    repair_part_t* grabbed_part = repair_part_get(scene->grabbed_part);

    scene->grabbed_part = 0;

    if (!grabbed_part) {
        return false;
    }

    if (!repair_scene_is_in_right_spot(scene, grabbed_part)) {
        return false;
    }

    repair_part_connect(grabbed_part);

    return true;
}

void repair_scene_exit_with_message(repair_scene_t* scene, const char* message) {
    if (!scene->is_active) {
        return;
    }

    scene->is_active = false;

    cutscene_builder_t builder;
    cutscene_builder_init(&builder);

    if (message) {
        cutscene_builder_delay(&builder, 1.0f);
        cutscene_builder_dialog(&builder, message);
    }
    cutscene_builder_fade(&builder, FADE_COLOR_BLACK, 1.0f);
    cutscene_builder_delay(&builder, 1.0f);
    cutscene_builder_load_scene(&builder, scene->exit_scene);
    cutscene_builder_fade(&builder, FADE_COLOR_NONE, 1.0f);

    cutscene_runner_run(cutscene_builder_finish(&builder), 0, cutscene_runner_free_on_finish(), NULL, 0);
}

void repair_scene_update(void *data) {
    repair_scene_t* repair_scene = (repair_scene_t*)data;

    joypad_inputs_t input = joypad_get_inputs(0);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    repair_scene->screen_cursor.x += input_handle_deadzone(input.stick_x) * (CURSOR_SPEED / 80) * fixed_time_step;
    repair_scene->screen_cursor.y -= input_handle_deadzone(input.stick_y) * (CURSOR_SPEED / 80) * fixed_time_step;

    repair_scene->screen_cursor.x = clampf(repair_scene->screen_cursor.x, 0.0f, SCREEN_WD - 1);
    repair_scene->screen_cursor.y = clampf(repair_scene->screen_cursor.y, 0.0f, SCREEN_HT - 1);

    if (repair_part_get(repair_scene->last_raycast_result)) {
        repair_scene->hovered_part = repair_scene->last_raycast_result;
    } else {
        repair_scene->hovered_part = 0;
    }
    
    if (pressed.a || (pressed.b && repair_scene->grabbed_part)) {
        if (repair_scene->grabbed_part) {
            if (repair_scene_check_drop(repair_scene)) {
                // audio_play_2d(scene->assets.sounds[REPAIR_SOUND_CLICK], 1.0f, 0.0f, 1.0f, 1);
            } else {
                // audio_play_2d(scene->assets.sounds[REPAIR_SOUND_PICKUP], 1.0f, 0.0f, 1.0f, 1);
            }
        } else {
            if (repair_scene->hovered_part) {
                // audio_play_2d(scene->assets.sounds[REPAIR_SOUND_PICKUP], 1.0f, 0.0f, 1.0f, 1);
            }

            repair_scene->grabbed_part = repair_scene->hovered_part;
        }
    }

    if (repair_scene->grabbed_part) {
        repair_scene_handle_grabbed_part(repair_scene, input, pressed);
        bool new_can_drop = repair_scene_is_in_right_spot(repair_scene, repair_part_get(repair_scene->grabbed_part));

        if (new_can_drop && !repair_scene->can_drop) {
            // audio_play_2d(scene->assets.sounds[REPAIR_SOUND_HOVER], 1.0f, 0.0f, 1.0f, 1);
        }

        repair_scene->can_drop = new_can_drop;
    } else {
        repair_scene->can_drop = false;
    }

    if (repair_scene->is_active) {
        repair_part_status_t status = repair_part_status();
    
        switch (status) {
            case REPAIR_MISSING_PARTS:
                repair_scene_exit_with_message(repair_scene, "You are missing some parts");
                break;
            case REPAIR_INCOMPLETE:
                break;
            case REPAIR_COMPLETE:
                repair_scene_exit_with_message(repair_scene, "Repair complete");
                expression_set_bool(repair_scene->puzzle_complete, true);
                break;
        }
    }

    if (pressed.start) {
        repair_scene_exit_with_message(repair_scene, NULL);
        return;
    }
}

void repair_scene_render_menu(void* data) {
    repair_scene_t* repair_scene = (repair_scene_t*)data;

    screen_raycast_read_result(&repair_scene->last_raycast_result);

    repair_scene_render_cursor(repair_scene);
}
    
void repair_scene_init(repair_scene_t* repair_scene, struct repair_scene_definition* definition, entity_id entity_id) {
    camera_init(&repair_scene->camera, definition->fov, WORLD_NEAR_PLANE, WORLD_FAR_PLANE);
    transformInit(&repair_scene->camera.transform, &definition->position, &definition->rotation, &gOneVec);
    player_disable(&current_scene->player);
    render_scene_use_camera(&repair_scene->camera);

    update_add(repair_scene, repair_scene_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    menu_add_callback(repair_scene_render_menu, repair_scene, MENU_PRIORITY_HUD);

    repair_scene->screen_cursor = (vector2_t){SCREEN_WD / 2, SCREEN_HT / 2};

    repair_scene->grabbed_part = 0;
    repair_scene->hovered_part = 0;
    repair_scene->puzzle_complete = definition->puzzle_complete;

    repair_scene->last_raycast_result = 0;
    repair_scene->exit_scene = definition->exit_scene;

    render_scene_add(NULL, 0.0f, repair_scene_render_callback, repair_scene);

    repair_scene->is_active = true;
    repair_scene->can_drop = false;
}

void repair_scene_destroy(repair_scene_t* repair_scene, struct repair_scene_definition* definition) {
    render_scene_remove_camera(&repair_scene->camera);

    update_remove(repair_scene);

    menu_remove_callback(repair_scene);
    render_scene_remove(repair_scene);
}

void repair_scene_common_init() {
    material_load_file(&cursor_material, "rom:/materials/repair/cursor.mat");
}

void repair_scene_common_destroy() {
    material_release(&cursor_material);
}
