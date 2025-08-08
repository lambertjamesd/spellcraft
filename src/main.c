#include <stdio.h>

#include <t3d/t3d.h>

#include "resource/material_cache.h"
#include "resource/sprite_cache.h"
#include "render/camera.h"
#include "math/transform.h"
#include "menu/spell_building_menu.h"
#include "savefile/savefile.h"
#include "time/time.h"
#include "collision/collision_scene.h"
#include "menu/menu_rendering.h"
#include "render/render_scene.h"

#include "render/render_batch.h"
#include "scene/scene_loader.h"
#include "objects/crate.h"
#include "time/game_mode.h"
#include "render/tmesh.h"
#include "util/init.h"
#include "util/screen_debug.h"

#include <libdragon.h>
#include <n64sys.h>

#define RDPQ_VALIDATE_DETACH_ADDR    0x00800000

void setup() {
    debug_init_isviewer();
    // fprintf(stderr, "This is how to talk");
    init_engine();
    savefile_new();

    scene_queue_next("rom:/scenes/StartArea_TempleOutside.scene");
    // scene_queue_next("rom:/scenes/overworld_test.scene");
    // scene_queue_next("rom:/scenes/ability_testing.scene");
    // scene_queue_next("rom:/scenes/playerhome_outside.scene");

    current_scene = scene_load(scene_get_next());

    scene_clear_next();
    screen_debug_init();
}

static struct frame_memory_pool frame_memory_pools[2];
static uint8_t next_frame_memoy_pool;

void render_3d() {
    uint8_t colorAmbient[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    t3d_frame_start();

    t3d_screen_clear_color(RGBA32(100, 0, 100, 0));
    t3d_screen_clear_depth();

    t3d_light_set_ambient(colorAmbient); // one global ambient light, always active
    t3d_light_set_count(0);
    
    struct frame_memory_pool* pool = &frame_memory_pools[next_frame_memoy_pool];
    frame_pool_reset(pool);

    T3DViewport* viewport = frame_malloc(pool, sizeof(T3DViewport));
    *viewport = t3d_viewport_create();

    render_scene_render(&current_scene->camera, viewport, &frame_memory_pools[next_frame_memoy_pool]);
    
    next_frame_memoy_pool ^= 1;
}

void render_menu() {
    rdpq_mode_persp(false);
    rdpq_set_mode_standard();
    menu_render();
    screen_debug_render();
}

void render(surface_t* zbuffer) {
    update_render_time();

    if (current_game_mode == GAME_MODE_3D || current_game_mode == GAME_MODE_TRANSITION_TO_MENU) {
        render_3d();
    } else if (current_game_mode == GAME_MODE_MENU) {
        static surface_t background;
        background = surface_make_linear(zbuffer->buffer, FMT_RGBA16, zbuffer->width, zbuffer->height);
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_TEX);
        rdpq_tex_blit(&background, 0, 0, NULL);
    }
    render_menu();
}

volatile static int frame_happened = 0;

void on_vi_interrupt() {
    frame_happened = 1;
}

bool check_scene_load() {
    static uint8_t frame_wait = 0;

    if (!scene_has_next()) {
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

    scene_release(current_scene);
    current_scene = scene_load(scene_get_next());
    scene_clear_next();

    return false;
}

#define DEBUG_CONNECT_DELAY     TICKS_FROM_MS(1500)

int main(void)
{
    display_init(RESOLUTION_320x240, DEPTH_32_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    t3d_init((T3DInitParams){});
    dfs_init(DFS_DEFAULT_LOCATION);
    joypad_init();

    surface_t zbuffer = surface_alloc(FMT_RGBA16, 320, 240);

    debug_init_usblog();
    console_set_debug(true);

    // give time for the debugger to connect
    long long start_time = timer_ticks();
    while (timer_ticks() - start_time < DEBUG_CONNECT_DELAY);

    setup();

    register_VI_handler(on_vi_interrupt);

    while(1) {
        while (!frame_happened) {
            // TODO process low priority tasks
        }
        frame_happened = 0;

        if (check_scene_load()) {
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
            
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER_TEX);
            rdpq_tex_blit(fb, 0, 0, NULL);

            rdpq_set_color_image(fb);

            render_menu();

            rdpq_detach_show();

            current_game_mode = GAME_MODE_MENU;
        } else {
            surface_t* fb = display_try_get();

            if (fb) {
                rdpq_attach(fb, &zbuffer);

                render(&zbuffer);

                rdpq_detach_show();
            } 
        }

        joypad_poll();
        if (update_has_layer(UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE)) {
            collision_scene_collide();
        }
        update_dispatch();
    }
}