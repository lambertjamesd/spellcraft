#include <stdio.h>


#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>

#include "resource/mesh_cache.h"
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

#include <libdragon.h>
#include <n64sys.h>

#define RDPQ_VALIDATE_DETACH_ADDR    0x00800000

struct world* current_world;

struct spell_symbol test_spell_symbols[] = {
    {.reserved = 0, .type = SPELL_SYMBOL_PUSH},
    {.reserved = 0, .type = SPELL_SYMBOL_PROJECTILE},
    {.reserved = 0, .type = SPELL_SYMBOL_FIRE},
};

struct spell test_spell = {
    .symbols = test_spell_symbols,
    .cols = 3,
    .rows = 1,
};

void setup() {
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
    current_world = world_load("rom:/worlds/desert.world");
}

float angle = 0.0f;

void render_3d() {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    static const float gold[13] = { 
        0.24725, 0.1995, 0.0745, 1.0,      /* ambient */
        0.75164, 0.60648, 0.22648, 1.0,    /* diffuse */
        0.628281, 0.555802, 0.366065, 1.0, /* specular */
        50.0                               /* shininess */
    };

    float goldGlow[4] = {0.2, 0.1, 0.05, 1};

    glEnable( GL_COLOR_MATERIAL );
    glEnable(GL_LIGHTING);
    glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, gold );
    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, &gold[4] );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &gold[8] );
    glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, goldGlow );
    glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, gold[12] );

    float blue1[4] = { 0.4, 0.4, 0.6, 1 };
    float blue2[4] = { 0.0, 0, 0.8, 1 };
    float blue3[4] = { 0.0, 0, 0.15, 1 };
    float pos[4] = {1, 0, 0, 0};
    glEnable( GL_LIGHT0 );
    glLightfv( GL_LIGHT0, GL_DIFFUSE, blue1 );
    glLightfv( GL_LIGHT0, GL_SPECULAR, blue2 );
    glLightfv( GL_LIGHT0, GL_AMBIENT, blue3 );
    glLightfv( GL_LIGHT0, GL_POSITION, pos);

    struct render_viewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.w = display_get_width();
    viewport.h = display_get_height();

    render_scene_render(&current_world->camera, &viewport);
}

void render_menu() {
    rdpq_mode_persp(false);
    rdpq_set_mode_standard();
    glDisable(GL_DEPTH_TEST);
    menu_render();
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
    gl_init();
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

            gl_context_begin();

            render_3d();

            gl_context_end();

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

                gl_context_begin();

                render(&zbuffer);

                gl_context_end();

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