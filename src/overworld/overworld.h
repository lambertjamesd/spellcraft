#ifndef _OVERWORLD_OVERWORLD_
#define _OVERWORLD_OVERWORLD_

#include <stdint.h>
#include <stdio.h>
#include "../math/vector2.h"
#include "../render/tmesh.h"

#include <t3d/t3d.h>

struct overworld_tile_def {
    uint32_t visual_block_offset;
    uint32_t actor_block_offset;
};

struct overworld_tile {
    struct tmesh terrain_mesh;
    struct tmesh** detail_meshes;
    uint16_t terrain_mesh_count;
    uint16_t detail_meshes_count;
    uint16_t detail_count;
    T3DMat4FP* detail_matrices;
    float starting_y;
    float scale_y;
};

struct overworld_tile_render_block {
    rspq_block_t* render_block;
    uint16_t x;
    uint16_t y;
    float starting_y;
    float scale_y;
};

#define NO_TILE_COORD   0xFFFF

struct overworld {
    uint16_t tile_x, tile_y;
    struct Vector2 min;
    float inv_tile_size;
    float tile_size;
    struct tmesh lod_0_meshes[4];
    struct overworld_tile_def* tile_definitions;
    FILE* file;

    // tile locations modulo wrap to share slots
    struct overworld_tile* loaded_tiles[4][4];
    struct overworld_tile_render_block render_blocks[4][4];

    struct { uint16_t x; uint16_t y; } load_next;
};

struct overworld* overworld_load(const char* filename);
void overworld_free(struct overworld* overworld);

#endif