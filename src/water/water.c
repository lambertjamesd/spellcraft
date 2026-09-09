#include "water.h"

#include <libdragon.h>
#include "../render/render_scene.h"
#include "../render/defs.h"
#include "../menu/menu_rendering.h"

static uint32_t WATER_OVERLAY_ID = 0;

#define PROCESS_BLOCK       0
#define PROCESS_PROFILE     1
#define PROCESS_APPLY       2

#define SIM_BUFFER_SIZE     1088
#define MAX_VERT_PER_CHUNK  (SIM_BUFFER_SIZE / 16)

#define SIM_SIZE            32
#define PIXEL_COUNT         (SIM_SIZE * SIM_SIZE)
#define PADDED_PIXEL_COUNT  (PIXEL_COUNT + SIM_SIZE * 2)
#define Y_STRIDE            (SIM_BUFFER_SIZE / (SIM_SIZE * sizeof(int16_t)) - 1)

#define SIM_WORLD_SIZE      16 

DEFINE_RSP_UCODE(rsp_water);

struct water_simulation {
    int16_t* velocity_buffer;
    int8_t* position_buffers[2];
    uint16_t ref_count;
    uint8_t read_buffer;
    bool is_dirty;
    vector2s16_t min;
    vector2s16_t next_min;
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
    simulation.min = (vector2s16_t){};
    simulation.next_min = (vector2s16_t){};
    simulation.scale = (vector2s16_t){{{0x10000 * SIM_SIZE / (SIM_WORLD_SIZE * MODEL_SCALE), 0x10000 * SIM_SIZE / (SIM_WORLD_SIZE * MODEL_SCALE)}}};

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
    int16_t* vel_out = simulation.velocity_buffer;
    int8_t* in = simulation.position_buffers[simulation.read_buffer];
    int8_t* out = simulation.position_buffers[write_index] + SIM_SIZE * sizeof(int8_t);

    int x_size = SIM_SIZE;
    int y_size = SIM_SIZE;

    vector2s16_t diff;
    vector2s16Sub(&simulation.next_min, &simulation.min, &diff);

    if (diff.x > 0) {
        out += diff.x;
        vel_out += diff.x;
        x_size -= diff.x;
    } else {
        in -= diff.x;
        vel -= diff.x;
        x_size += diff.x;
    }

    int block_y_stride = SIM_SIZE * Y_STRIDE;

    if (diff.y > 0) {
        out += diff.y * SIM_SIZE;
        vel_out += diff.y * SIM_SIZE;
        y_size -= diff.y;

        if (y_size > Y_STRIDE) {
            int start_offset = (y_size - Y_STRIDE) * SIM_SIZE;

            in += start_offset;
            out += start_offset;
            vel += start_offset;
            vel_out += start_offset;
            block_y_stride = -block_y_stride;
        }
    } else {
        in -= diff.y * SIM_SIZE;
        vel -= diff.y * SIM_SIZE;
        y_size += diff.y;
    }

    for (int y = 0; y < y_size && x_size > 0; y += Y_STRIDE) {
        int y_count = Y_STRIDE;
        int rows_remaining = y_size - y;

        if (y_count > rows_remaining) {
            y_count = rows_remaining;
        }

        rspq_write_t write = rspq_write_begin(WATER_OVERLAY_ID, PROCESS_BLOCK, 5);
        rspq_write_arg(&write, ((int)y_count << Y_STRIDE_OFFSET) | x_size); 
        rspq_write_arg(&write, PhysicalAddr(vel));
        rspq_write_arg(&write, PhysicalAddr(in));
        rspq_write_arg(&write, PhysicalAddr(vel_out));
        rspq_write_arg(&write, PhysicalAddr(out));
        rspq_write_end(&write);

        if (block_y_stride < 0 && rows_remaining < Y_STRIDE * 2) {
            int next_rows_remaining = rows_remaining - Y_STRIDE;
            block_y_stride = -next_rows_remaining * SIM_SIZE;
        }
        
        vel += block_y_stride;

        in += block_y_stride;
        out += block_y_stride;
        vel_out += block_y_stride;
    }

    simulation.read_buffer = write_index;
    simulation.min = simulation.next_min;
}

void water_simulation_apply(tmesh_t* mesh, vector3_t* position) {
    if (simulation.is_dirty) {
        water_simulation_update();
    }

    vector2s16_t min;
    water_simulation_rounded_position(position, &min);
    vector2s16Sub(&simulation.min, &min, &min);

    for (int vtx_offset = 0; vtx_offset < mesh->vertex_count; vtx_offset += MAX_VERT_PER_CHUNK) {
        rspq_write_t write = rspq_write_begin(WATER_OVERLAY_ID, PROCESS_APPLY, 5);

        int remaining = mesh->vertex_count - vtx_offset;

        if (remaining > MAX_VERT_PER_CHUNK) {
            remaining = MAX_VERT_PER_CHUNK; 
        }
    
        rspq_write_arg(&write, remaining);
        rspq_write_arg(&write, (int)PhysicalAddr(simulation.position_buffers[simulation.read_buffer] + SIM_SIZE));
        rspq_write_arg(&write, PhysicalAddr(mesh->vertices + vtx_offset));
        rspq_write_arg(&write, min.equalTest);
        rspq_write_arg(&write, simulation.scale.equalTest);
    
        rspq_write_end(&write);
    }
}


void water_simulation_debug_render(void* data) {
    rdpq_set_combiner_raw(RDPQ_COMBINER1((0, 0, 0, TEX0), (0, 0, 0, 1)));
    
    rdpq_sprite_upload(TILE0, sprite_test, NULL);

    surface_t surface;

    surface.buffer = simulation.position_buffers[1] + SIM_SIZE;
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

void water_simulation_set_center(vector3_t* position) {
    water_simulation_rounded_position(position, &simulation.next_min);
    simulation.next_min.x = (simulation.next_min.x - SIM_SIZE / 2 + 4) & ~0x7; 
    simulation.next_min.y = simulation.next_min.y - SIM_SIZE / 2;
}

void water_simulation_set_callback(void* data) {
    int args = (int)data;

    int8_t value = (int8_t)(uint8_t)args;
    uint8_t sim_radius = (uint8_t)(args >> 8);
    int8_t y_center = (int8_t)(uint8_t)(args >> 16);
    int8_t x_center = (int8_t)(uint8_t)(args >> 24);

    int min_x = x_center - sim_radius;
    int min_y = y_center - sim_radius;
    int max_x = x_center + sim_radius;
    int max_y = y_center + sim_radius;

    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x > SIM_SIZE) max_x = SIM_SIZE;
    if (max_y > SIM_SIZE) max_y = SIM_SIZE;

    int8_t* buffer = UncachedAddr(simulation.position_buffers[simulation.read_buffer] + SIM_SIZE);

    for (int y = min_y; y < max_y; y += 1) {
        for (int x = min_x; x < max_x; x += 1) {
            buffer[y * SIM_SIZE + x] = value;
        }
    }
}

void water_simulation_set(vector3_t* position, float radius, int8_t value) {
    if (simulation.ref_count == 0) {
        return;
    }
    
    vector2s16_t sim_pos;
    water_simulation_rounded_position(position, &sim_pos);
    vector2s16Sub(&sim_pos, &simulation.min, &sim_pos);

    int sim_radius = (int)ceilf(radius * ((float)SIM_SIZE / (float)SIM_WORLD_SIZE));

    if (sim_pos.x < sim_radius || 
        sim_pos.y < sim_radius || 
        sim_pos.x > SIM_SIZE + sim_radius ||
        sim_pos.y > SIM_SIZE + sim_radius
    ) {
        return;
    }

    int args = ((int)(sim_pos.x & 0xFF) << 24) | ((int)(sim_pos.y & 0xFF) << 16) | ((sim_radius & 0xFF) << 8) | ((int)(uint8_t)value & 0xFF);
    rspq_call_deferred(water_simulation_set_callback, (void*)args);
}

void water_simulation_rounded_position(vector3_t* input, vector2s16_t* output) {
    output->x = roundf(input->x * ((float)SIM_SIZE / (float)SIM_WORLD_SIZE));
    output->y = roundf(input->z * ((float)SIM_SIZE / (float)SIM_WORLD_SIZE));
}