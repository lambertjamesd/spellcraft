#include "water_waves.h"    

#include "../menu/menu_rendering.h"

void water_waves_debug_render(void* data) {
    water_waves_t* water_waves = (water_waves_t*)data;

    rdpq_set_combiner_raw(RDPQ_COMBINER1((0, 0, 0, TEX0), (0, 0, 0, 1)));

    surface_t surface;

    surface.buffer = water_waves->simulation.position_buffers[water_waves->simulation.read_buffer];
    surface.width = 32;
    surface.height = 32;
    surface.stride = 64;
    surface.flags = FMT_IA16;

    rdpq_tex_upload(TILE0, &surface, NULL);
    rdpq_set_tile(
        TILE0, 
        FMT_IA16, 
        0, 
        ((TEX_FORMAT_PIX2BYTES(FMT_IA16, 32)) + 0x7) & ~0x7, 
        NULL
    );

    rdpq_set_tile_size_fx(TILE0, 0, 0, 124, 124);

    rdpq_texture_rectangle(TILE0, 20, 20, 52, 52, 0, 0);
}
    
void water_waves_init(water_waves_t* water_waves, struct water_waves_definition* definition, entity_id entity_id) {
    water_waves->position = definition->position;
    water_simulation_init(&water_waves->simulation, definition->width, definition->height);

    menu_add_callback(water_waves_debug_render, water_waves, MENU_PRIORITY_OVERLAY);

    int16_t* pix = UncachedAddr(water_waves->simulation.position_buffers[0]);

    for (int y = 1; y < 31; y += 1) {
        for (int x = 1; x < 31; x += 1) {
            pix[y * 32 + x] = 0x7f7f;
        }
    }
}

void water_waves_destroy(water_waves_t* water_waves, struct water_waves_definition* definition) {
    water_simulation_destroy(&water_waves->simulation);
    menu_remove_callback(water_waves);
}

void water_waves_common_init() {

}

void water_waves_common_destroy() {

}
