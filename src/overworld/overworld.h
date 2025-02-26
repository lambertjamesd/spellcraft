#ifndef _OVERWORLD_OVERWORLD_
#define _OVERWORLD_OVERWORLD_

#include <stdint.h>
#include "../math/matrix.h"
#include "../math/vector2.h"
#include "../render/frame_alloc.h"
#include "../render/tmesh.h"

struct overworld_tile_def {
    uint32_t rom_location;
};

struct overworld_tile {
    struct tmesh** detail_meshes;
    uint16_t detail_meshes_count;
    uint16_t detail_count;
    T3DMat4FP* detail_matrices;

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
    struct Vector2 inv_tile_size;

    // tile locations modulo wrap to share slots
    struct overworld_tile* loaded_tiles[4][4];
    struct overworld_tile_render_block render_blocks[4][4];

    struct overworld_tile_def* tile_definitions;
};

void overworld_render(struct overworld* overworld, mat4x4 view_proj_matrix, struct frame_memory_pool* pool);

#endif