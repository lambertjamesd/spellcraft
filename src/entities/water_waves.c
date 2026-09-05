#include "water_waves.h"    

#include "../menu/menu_rendering.h"

static sprite_t* sprite_test;
static tmesh_t* mesh_test;
static water_index_range_t ranges[1];
static water_apply_args_t args = {
    .index_ranges = ranges,
    .index_range_count = 1,
    .min = {{{0, 0}}},
    .scale = {{{0xFFFF, 0xFFFF}}},
};

void water_waves_debug_render(void* data) {
    water_waves_t* water_waves = (water_waves_t*)data;

    if (joypad_get_buttons_pressed(0).z) {
        int8_t* pix = water_waves->simulation.position_buffers[water_waves->simulation.read_buffer];

        for (int y = 4; y < 8; y += 1) {
            for (int x = 4; x < 8; x += 1) {
                pix[y * 32 + x] = 0x7F;
            }
        }

        data_cache_hit_writeback_invalidate(pix, sizeof(int8_t) * 32 * 32);
    }

    water_simulation_update(&water_waves->simulation);
    // water_simulation_apply(&water_waves->simulation, &args);

    rdpq_set_combiner_raw(RDPQ_COMBINER1((0, 0, 0, TEX0), (0, 0, 0, 1)));
    
    rdpq_sprite_upload(TILE0, sprite_test, NULL);

    surface_t surface;

    surface.buffer = water_waves->simulation.position_buffers[1] + water_waves->simulation.width;
    surface.flags = FMT_I8;
    surface.width = 32;
    surface.height = 32;
    surface.stride = 32;

    rdpq_texparms_t texparms = (rdpq_texparms_t){};
    texparms.s.repeats = REPEAT_INFINITE;
    texparms.t.repeats = REPEAT_INFINITE;
    rdpq_tex_upload(TILE0, &surface, &texparms);
    rdpq_tileparms_t tileparms = (rdpq_tileparms_t){};
    rdpq_set_tile(
        TILE0, 
        FMT_I8, 
        0, 
        (TEX_FORMAT_PIX2BYTES(FMT_I8, 32) + 0x7) & ~0x7, 
        &tileparms
    );

    rdpq_set_tile_size_fx(TILE0, 0, 0, 128, 128);

    rdpq_texture_rectangle(TILE0, 20, 20, 52, 52, 0, 0);
    
    surface.buffer = water_waves->simulation.velocity_buffer;
    surface.flags = FMT_IA16;
    surface.width = 32;
    surface.height = 32;
    surface.stride = 64;

    rdpq_tex_upload(TILE0, &surface, &texparms);
    rdpq_set_tile(
        TILE0, 
        FMT_IA16, 
        0, 
        (TEX_FORMAT_PIX2BYTES(FMT_IA16, 32) + 0x7) & ~0x7, 
        &tileparms
    );

    rdpq_set_tile_size_fx(TILE0, 0, 0, 128, 128);

    rdpq_texture_rectangle(TILE0, 64, 20, 96, 52, 0, 0);
}
    
void water_waves_init(water_waves_t* water_waves, struct water_waves_definition* definition, entity_id entity_id) {
    transformSaInit(&water_waves->transform, &definition->position, &gRight2, 1.0f);
    water_simulation_init(&water_waves->simulation, definition->width, definition->height);

    menu_add_callback(water_waves_debug_render, water_waves, MENU_PRIORITY_OVERLAY);

    int8_t* pix = water_waves->simulation.position_buffers[0];

    for (int y = 4; y < 8; y += 1) {
        for (int x = 14; x < 18; x += 1) {
            pix[y * 32 + x] = 0x7F;
        }
    }

    data_cache_hit_writeback_invalidate(pix, sizeof(int8_t) * 32 * 32);

    render_scene_init_add_renderable(&water_waves->renderable, &water_waves->transform, mesh_test, 1.0f);
}

void water_waves_destroy(water_waves_t* water_waves, struct water_waves_definition* definition) {
    water_simulation_destroy(&water_waves->simulation);
    menu_remove_callback(water_waves);
}

void water_waves_common_init() {
    sprite_test = sprite_load("rom:/images/test/ia_test.sprite");
    surface_t surf = sprite_get_pixels(sprite_test);

    debugf("water_waves_common_init %d %d %d %d\n", surf.flags, surf.width, surf.height, surf.stride);

    mesh_test = tmesh_cache_load("rom:/meshes/test/water_sim.tmesh");

    for (int i = 0; i < mesh_test->vertex_count; i += 1) {
        mesh_test->vertices[i].normA = 0;
        mesh_test->vertices[i].normB = 0;
    }

    args.vtx = mesh_test->vertices;
    ranges[0].min = 0;
    ranges[1].max = 16 * mesh_test->vertex_count;
}

void water_waves_common_destroy() {
    sprite_free(sprite_test);
    tmesh_cache_release(mesh_test);
}
