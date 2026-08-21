#include "repair_scene.h"    

#include "../scene/scene.h"
#include "../render/defs.h"
#include "../util/input.h"
#include "repair_part.h"

static material_t cursor_material;

#define CURSOR_SPEED    300.0f

#define CURSOR_SIZE 32

static color_t basic_color = {255, 255, 255, 255};
static color_t hover_color = {20, 200, 255, 255};

uint16_t __attribute__((aligned(16))) z_test[4] = {1, 2, 3, 4};

extern void* zbuffer_data;

void repair_scene_update_raycast_pos(repair_scene_t* scene) {
    int x = (int)scene->screen_cursor.x;
    int y = (int)scene->screen_cursor.y;

    int offset = (y * SCREEN_WD + x) * sizeof(uint16_t);
    repair_part_raycast_position = (uint8_t*)zbuffer_data + offset;
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

void repair_scene_update(void *data) {
    repair_scene_t* repair_scene = (repair_scene_t*)data;

    joypad_inputs_t input = joypad_get_inputs(0);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    repair_scene->screen_cursor.x += input_handle_deadzone(input.stick_x) * (CURSOR_SPEED / 80) * fixed_time_step;
    repair_scene->screen_cursor.y -= input_handle_deadzone(input.stick_y) * (CURSOR_SPEED / 80) * fixed_time_step;

    repair_scene->screen_cursor.x = clampf(repair_scene->screen_cursor.x, 0.0f, SCREEN_WD - 1);
    repair_scene->screen_cursor.y = clampf(repair_scene->screen_cursor.y, 0.0f, SCREEN_HT - 1);

    repair_scene_update_raycast_pos(repair_scene);

    repair_scene->hovered_part = repair_scene->last_raycast_result.id;

    debugf("last_id = %d\n", repair_scene->last_raycast_result.id);
}

void repair_scene_render_menu(void* data) {
    repair_scene_t* repair_scene = (repair_scene_t*)data;

    screen_raycast_read_entity(&repair_scene->last_raycast_result);

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
    repair_scene_update_raycast_pos(repair_scene);

    repair_scene->grabbed_part = 0;
    repair_scene->hovered_part = 0;

    repair_scene->last_raycast_result = (screen_raycast_result_t){};
}

void repair_scene_destroy(repair_scene_t* repair_scene, struct repair_scene_definition* definition) {
    render_scene_remove_camera(&repair_scene->camera);

    update_remove(repair_scene);

    menu_remove_callback(repair_scene);
}

void repair_scene_common_init() {
    material_load_file(&cursor_material, "rom:/materials/repair/cursor.mat");
    screen_raycast_init();
}

void repair_scene_common_destroy() {
    material_release(&cursor_material);
    screen_raycast_destroy();
}
