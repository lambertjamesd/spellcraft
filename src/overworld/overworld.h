#ifndef _OVERWORLD_OVERWORLD_
#define _OVERWORLD_OVERWORLD_

#include <stdint.h>
#include "../math/vector2.h"
#include "../render/tmesh.h"

#include <t3d/t3d.h>

struct overworld_tile_def {
    uint32_t rom_location;
};

struct overworld_tile {
    struct tmesh* terrain_meshes;
    struct tmesh** detail_meshes;
    uint16_t terrain_mesh_count;
    uint16_t detail_meshes_count;
    uint16_t detail_count;
    T3DMat4FP* detail_matrices;
    rspq_block_t* render_block;
};

struct overworld_tile_render_block {
    rspq_block_t* render_block;
    uint16_t x;
    uint16_t y;
};

struct overworld {
    uint16_t tile_x, tile_y;
    uint16_t tile_x_mask;
    struct Vector2 min;
    float inv_tile_size;
    float tile_size;

    // tile locations modulo wrap to share slots
    struct overworld_tile* loaded_tiles[4][4];
    struct overworld_tile_render_block render_blocks[4][4];
    struct overworld_tile* exit_queue[2];

    struct overworld_tile_def* tile_definitions;
};

#endif