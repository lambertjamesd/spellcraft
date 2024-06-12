#include <stdio.h>

#include <t3d/t3d.h>

#include "resource/material_cache.h"
#include "resource/sprite_cache.h"
#include "render/camera.h"
#include "math/transform.h"
#include "render/render_scene.h"
#include "spell/assets.h"
#include "collision/collision_scene.h"
#include "entity/health.h"
#include "menu/menu_rendering.h"
#include "menu/spell_building_menu.h"
#include "menu/menu_common.h"
#include "objects/collectable.h"
#include "menu/dialog_box.h"
#include "cutscene/cutscene_runner.h"
#include "entity/interactable.h"
#include "savefile/savefile.h"

#include "render/render_batch.h"
#include "scene/world_loader.h"
#include "time/time.h"
#include "objects/crate.h"
#include "time/game_mode.h"
#include "render/tmesh.h"

#include <libdragon.h>
#include <n64sys.h>

#define RDPQ_VALIDATE_DETACH_ADDR    0x00800000

struct world* current_world;

static T3DVertPacked __attribute__ ((aligned (16))) vertices[2];
struct tmesh tmesh_test;

void draw_test(void* data, struct render_batch* batch) {
    struct Transform transform;
    transformInitIdentity(&transform);
    mat4x4* mtx = render_batch_get_transform(batch);
    transformToMatrix(&transform, *mtx);
    render_batch_add_tmesh(batch, &tmesh_test, mtx, NULL);
}

void setup() {
    debug_init_isviewer();
    // fprintf(stderr, "This is how to talk");
    spell_assets_init();
    menu_common_init();
    render_scene_reset();
    update_reset();
    collision_scene_reset();
    health_reset();
    interactable_reset();
    menu_reset();
    collectable_assets_load();
    dialog_box_init();
    cutscene_runner_init();
    savefile_new();

    uint16_t norm = t3d_vert_pack_normal(&(T3DVec3){{ 0, 0, 1}}); // normals are packed in a 5.6.5 format
    vertices[0] = (T3DVertPacked){
        .posA = {-16, -16, 0}, .rgbaA = 0xFF0000FF, .normA = norm,
        .posB = { 16, -16, 0}, .rgbaB = 0x00FF00FF, .normB = norm,
    };
    vertices[1] = (T3DVertPacked){
        .posA = { 16,  16, 0}, .rgbaA = 0x0000FFFF, .normA = norm,
        .posB = {-16,  16, 0}, .rgbaB = 0xFF00FFFF, .normB = norm,
    };

    data_cache_hit_writeback(&vertices[0], sizeof(vertices));

    FILE* file = asset_fopen("rom:/meshes/objects/pickups/heart.tmesh", NULL);
    tmesh_load(&tmesh_test, file);
    fclose(file);

    render_scene_add(&gZeroVec, 1.0f, draw_test, NULL);

    current_world = world_load("rom:/worlds/playerhome_outside.world");
}

static float time = 0.0f;

static struct frame_memory_pool frame_memory_pools[2];
static uint8_t next_frame_memoy_pool;

void transform_to_t3d(struct Transform* transform, T3DMat4FP* matrix) {
    T3DMat4 tmp;
    transformToMatrix(transform, tmp.m);
    t3d_mat4_to_fixed(matrix, &tmp);
}

void render_3d() {
    T3DViewport viewport = t3d_viewport_create();

    uint8_t colorAmbient[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t colorDir[4]     = {0xFF, 0xFF, 0xFF, 0xFF};

    t3d_frame_start();

    t3d_screen_clear_color(RGBA32(100, 0, 100, 0));
    t3d_screen_clear_depth();

    t3d_light_set_ambient(colorAmbient); // one global ambient light, always active
    t3d_light_set_count(0);
    
    struct frame_memory_pool* pool = &frame_memory_pools[next_frame_memoy_pool];
    frame_pool_reset(pool);

    struct Camera camera;
    camera_init(&camera, 90.0f, 0.5f, 10.0f);
    camera.transform.position.z = 2.0f;
    camera.transform.position.x = sinf(time);
    time += 0.1f;

    // mat4x4 view_proj_matrix;

    // camera_apply(&camera, &viewport, NULL, view_proj_matrix);

    // t3d_viewport_attach(&viewport);

    // struct render_batch batch;
    // render_batch_init(&batch, &camera.transform, pool);

    // struct Transform transform;
    // transformInitIdentity(&transform);
    // mat4x4 mtx;
    // transformToMatrix(&transform, mtx);
    // render_batch_add_tmesh(&batch, &tmesh_test, &mtx, NULL);

    // render_batch_finish(&batch, view_proj_matrix, &viewport);

    render_scene_render(&current_world->camera, &viewport, &frame_memory_pools[next_frame_memoy_pool]);
    frame_pool_finish(pool);
    
    next_frame_memoy_pool ^= 1;
}

void render_menu() {
    // rdpq_mode_persp(false);
    // rdpq_set_mode_standard();
    // menu_render();
}

void render(surface_t* zbuffer) {
    if (current_game_mode == GAME_MODE_3D || current_game_mode == GAME_MODE_TRANSITION_TO_MENU) {
        render_3d();
    } else if (current_game_mode == GAME_MODE_MENU) {
        surface_t background = *zbuffer;
        background.flags = FMT_RGBA16;
        rdpq_set_mode_copy(false);
        rdpq_tex_blit(&background, 0, 0, NULL);
    }
    render_menu();
}

volatile static int frame_happened = 0;

void on_vi_interrupt() {
    frame_happened = 1;
}

bool check_world_load() {
    static uint8_t frame_wait = 0;

    if (!world_has_next()) {
        return false;
    }

    if (frame_wait == 0) {
        frame_wait = 2;
        return true;
    } 

    --frame_wait;

    if (frame_wait > 0) {
        return true;
    }

    world_release(current_world);
    current_world = world_load(world_get_next());
    world_clear_next();

    return false;
}

int main(void)
{
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    t3d_init((T3DInitParams){});
    dfs_init(DFS_DEFAULT_LOCATION);
    joypad_init();

    surface_t zbuffer = surface_alloc(FMT_RGBA16, 320, 240);

    debug_init_usblog();
    console_set_debug(true);

    for (;;) {
        joypad_poll();

        if (joypad_get_buttons_pressed(0).start) {
            break;
        }
    }

    setup();

    register_VI_handler(on_vi_interrupt);

    while(1) {
        while (!frame_happened) {
            // TODO process low priority tasks
        }
        frame_happened = 0;

        if (check_world_load()) {
            continue;
        }

        if (current_game_mode == GAME_MODE_TRANSITION_TO_MENU) {
            surface_t* fb = display_get();

            rdpq_attach(fb, &zbuffer);

            render_3d();

            // copy the frame buffer into the z buffer
            // to be used as the background while the game
            // is paused
            rdpq_set_color_image_raw(
                0, 
                PhysicalAddr(zbuffer.buffer), 
                FMT_RGBA16, 
                zbuffer.width, 
                zbuffer.height, 
                zbuffer.stride
            );

            rdpq_set_mode_copy(false);
            rdpq_tex_blit(fb, 0, 0, NULL);

            rdpq_set_color_image(fb);

            render_menu();

            rdpq_detach_show();

            current_game_mode = GAME_MODE_MENU;
        } else {
            surface_t* fb = display_try_get();

            if (fb) {
                rdpq_attach(fb, current_game_mode == GAME_MODE_MENU ? NULL : &zbuffer);

                render(&zbuffer);

                rdpq_detach_show();
            } 
        }

        joypad_poll();
        if (update_has_layer(UPDATE_LAYER_WORLD)) {
            collision_scene_collide();
        }
        update_dispatch();
    }
}