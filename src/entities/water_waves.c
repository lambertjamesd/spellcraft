#include "water_waves.h"    

#include "../menu/menu_rendering.h"

static sprite_t* sprite_test;

void water_waves_debug_render(void* data) {
    water_waves_t* water_waves = (water_waves_t*)data;

    water_simulation_update(&water_waves->simulation);

    rdpq_set_combiner_raw(RDPQ_COMBINER1((0, 0, 0, TEX0), (0, 0, 0, 1)));
    
    rdpq_sprite_upload(TILE0, sprite_test, NULL);

    surface_t surface;

    surface.buffer = water_waves->simulation.position_buffers[1];
    surface.flags = FMT_IA16;
    surface.width = 32;
    surface.height = 32;
    surface.stride = 64;

    rdpq_texparms_t texparms = (rdpq_texparms_t){};
    texparms.s.repeats = REPEAT_INFINITE;
    texparms.t.repeats = REPEAT_INFINITE;
    rdpq_tex_upload(TILE0, &surface, &texparms);
    rdpq_tileparms_t tileparms = (rdpq_tileparms_t){};
    rdpq_set_tile(
        TILE0, 
        FMT_IA16, 
        0, 
        (TEX_FORMAT_PIX2BYTES(FMT_IA16, 32) + 0x7) & ~0x7, 
        &tileparms
    );

    rdpq_set_tile_size_fx(TILE0, 0, 0, 128, 128);

    rdpq_texture_rectangle(TILE0, 20, 20, 52, 52, 0, 0);
}
    
void water_waves_init(water_waves_t* water_waves, struct water_waves_definition* definition, entity_id entity_id) {
    water_waves->position = definition->position;
    water_simulation_init(&water_waves->simulation, definition->width, definition->height);

    menu_add_callback(water_waves_debug_render, water_waves, MENU_PRIORITY_OVERLAY);

    int16_t* pix = water_waves->simulation.velocity_buffer;

    for (int y = 3; y < 10; y += 1) {
        pix[y * 32 + 6] = 0x7000;
        // for (int x = 6; x < 7; x += 1) {
        // }
    }

    data_cache_hit_writeback_invalidate(pix, sizeof(int16_t) * 32 * 32);
}

void water_waves_destroy(water_waves_t* water_waves, struct water_waves_definition* definition) {
    water_simulation_destroy(&water_waves->simulation);
    menu_remove_callback(water_waves);
}

void water_waves_common_init() {
    sprite_test = sprite_load("rom:/images/test/ia_test.sprite");
    surface_t surf = sprite_get_pixels(sprite_test);

    debugf("water_waves_common_init %d %d %d %d\n", surf.flags, surf.width, surf.height, surf.stride);
}

void water_waves_common_destroy() {
    sprite_free(sprite_test);
}
