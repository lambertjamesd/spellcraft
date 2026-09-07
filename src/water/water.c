#include "water.h"

#include <libdragon.h>
#include "../render/render_scene.h"
#include "../menu/menu_rendering.h"

static uint32_t WATER_OVERLAY_ID = 0;

#define PROCESS_BLOCK       0
#define PROCESS_PROFILE     1
#define PROCESS_APPLY       2

#define SIM_BUFFER_SIZE     1024
#define MAX_VERT_PER_CHUNK  (SIM_BUFFER_SIZE / 16)

#define SIM_WIDTH           32
#define SIM_HEIGHT          32
#define PIXEL_COUNT         (SIM_WIDTH * SIM_HEIGHT)
#define PADDED_PIXEL_COUNT  (PIXEL_COUNT + SIM_WIDTH * 2)
#define Y_STRIDE            (SIM_BUFFER_SIZE / (SIM_WIDTH * sizeof(int16_t)) - 1)

DEFINE_RSP_UCODE(rsp_water);

struct water_simulation {
    int16_t* velocity_buffer;
    int8_t* position_buffers[2];
    uint16_t ref_count;
    uint8_t read_buffer;
    bool is_dirty;
    vector2s16_t min;
    vector2s16_t scale;
};

typedef struct water_simulation water_simulation_t;

static sprite_t* sprite_test;
static water_simulation_t simulation;

void water_simulation_render_start(void* data, mat4x4 view_proj_matrix, struct Camera* camera, T3DViewport* viewport, struct frame_memory_pool* pool) {
    simulation.is_dirty = true;
}

void water_simulation_retain() {
    ++simulation.ref_count;

    if (simulation.ref_count > 1) {
        return;
    }

    WATER_OVERLAY_ID = rspq_overlay_register(&rsp_water);

    int total_size = 2 * PADDED_PIXEL_COUNT * sizeof(uint8_t) + PIXEL_COUNT * sizeof(uint16_t);

    simulation.velocity_buffer = malloc(total_size);
    simulation.position_buffers[0] = (int8_t*)(simulation.velocity_buffer + PIXEL_COUNT);
    simulation.position_buffers[1] = simulation.position_buffers[0] + PADDED_PIXEL_COUNT;
    simulation.read_buffer = 0;
    simulation.scale = (vector2s16_t){{{0x0400, 0x0400}}};

    memset(simulation.velocity_buffer, 0, total_size);

    render_scene_add_step(water_simulation_render_start, &simulation);
}

void water_simulation_release() {
    --simulation.ref_count;

    if (simulation.ref_count) {
        return;
    }

    rspq_overlay_unregister(WATER_OVERLAY_ID);
    free(simulation.velocity_buffer);
    simulation.velocity_buffer = NULL;
    render_scene_remove_step(&simulation);

}

// 12 is chosen to simplify calculating dma transfer sizes
#define Y_STRIDE_OFFSET 12

void water_simulation_update() {
    int write_index = 1 - simulation.read_buffer;

    int16_t* vel = simulation.velocity_buffer;
    int8_t* in = simulation.position_buffers[simulation.read_buffer];
    int8_t* out = simulation.position_buffers[write_index];

    int block_y_stride = SIM_WIDTH * Y_STRIDE;
    int simluation_stride = SIM_WIDTH * sizeof(int8_t);
    
    for (int y = 1; y + 1 < SIM_HEIGHT; y += Y_STRIDE) {
        int y_count = Y_STRIDE;
        int rows_remaining = SIM_HEIGHT - y - 1;

        if (y_count > rows_remaining) {
            y_count = rows_remaining;
        }

        rspq_write(
            WATER_OVERLAY_ID, 
            PROCESS_BLOCK, 
            ((int)y_count << Y_STRIDE_OFFSET) | simluation_stride, 
            PhysicalAddr(vel), 
            PhysicalAddr(in), 
            PhysicalAddr(out + simluation_stride)
        );

        vel += block_y_stride;
        in += block_y_stride;
        out += block_y_stride;
    }

    simulation.read_buffer = write_index;
}

void water_simulation_apply(tmesh_t* mesh, vector3_t* position) {
    if (simulation.is_dirty) {
        water_simulation_update();
    }

    for (int vtx_offset = 0; vtx_offset < mesh->vertex_count; vtx_offset += MAX_VERT_PER_CHUNK) {
        rspq_write_t write = rspq_write_begin(WATER_OVERLAY_ID, PROCESS_APPLY, 5);

        int remaining = mesh->vertex_count - vtx_offset;

        if (remaining > MAX_VERT_PER_CHUNK) {
            remaining = MAX_VERT_PER_CHUNK; 
        }
    
        rspq_write_arg(&write, remaining);
        rspq_write_arg(&write, (int)PhysicalAddr(simulation.position_buffers[simulation.read_buffer]));
        rspq_write_arg(&write, PhysicalAddr(mesh->vertices + vtx_offset));
        rspq_write_arg(&write, simulation.min.equalTest);
        rspq_write_arg(&write, simulation.scale.equalTest);
    
        rspq_write_end(&write);
    }
}


void water_simulation_debug_render(void* data) {
    rdpq_set_combiner_raw(RDPQ_COMBINER1((0, 0, 0, TEX0), (0, 0, 0, 1)));
    
    rdpq_sprite_upload(TILE0, sprite_test, NULL);

    surface_t surface;

    surface.buffer = simulation.position_buffers[1] + SIM_WIDTH;
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
    
    surface.buffer = simulation.velocity_buffer;
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

void water_simulation_enable_debug_render() {
    menu_add_callback(water_simulation_debug_render, &simulation, MENU_PRIORITY_OVERLAY);
    sprite_test = sprite_load("rom:/images/test/ia_test.sprite");
}

void water_simulation_disable_debug_render() {
    menu_remove_callback(&simulation);
    sprite_free(sprite_test);
}