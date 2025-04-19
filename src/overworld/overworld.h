#ifndef _OVERWORLD_OVERWORLD_
#define _OVERWORLD_OVERWORLD_

#include "../collision/mesh_collider.h"
#include "../cutscene/expression.h"
#include "../math/vector2.h"
#include "../render/tmesh.h"
#include "../util/hash_map.h"
#include <stdint.h>
#include <stdio.h>
#include <t3d/t3d.h>

struct overworld_tile_def {
    uint32_t visual_block_offset;
    uint32_t actor_block_offset;
};

struct overworld_tile {
    struct tmesh* terrain_meshes;
    rspq_block_t** render_blocks;
    float starting_y;
    struct tmesh** detail_meshes;
    uint16_t terrain_mesh_count;
    uint16_t detail_mesh_count;
    uint16_t detail_count;
    T3DMat4FP* detail_matrices;
};

struct overworld_actor_spawn_information {
    uint16_t entity_type_id;
    struct expression expression;
    void* entity_def;
};

struct overworld_actor_spawn_location {
    uint8_t x, y;
    uint16_t spawn_id_offset;
};

struct overworld_actor {
    struct overworld_actor* next;
    uint32_t x, y;
    void* entity;
    uint16_t entity_type_id;
    uint16_t spawn_id_offset;
};

struct overworld_actor_tile {
    struct mesh_collider collider;
    uint16_t x;
    uint16_t y;

    uint16_t active_spawn_locations;
    
    uint32_t first_spawn_id;

    struct overworld_actor_spawn_location* spawn_locations;
    struct overworld_actor_spawn_information* spawn_information;
    char* string_table;
};

struct overworld_tile_render_block {
    rspq_block_t** render_blocks;
    float starting_y;
    uint8_t x;
    uint8_t z;
    uint8_t y_height;
};

struct overworld_lod0_entry {
    struct tmesh mesh;
    int16_t x, z;
    uint16_t priority;
};

struct overworld_lod0 {
    struct overworld_lod0_entry* entries;
    uint8_t entry_count;
};

#define NO_TILE_COORD   0xFFFF
#define MAX_ACTIVE_ACTORS   128

struct overworld {
    uint16_t tile_x, tile_y;
    struct Vector2 min;
    float inv_tile_size;
    float tile_size;
    struct overworld_lod0 lod0;
    struct overworld_tile_def* tile_definitions;
    FILE* file;

    // tile locations modulo wrap to share slots
    struct overworld_tile* loaded_tiles[4][4];
    struct overworld_tile_render_block render_blocks[4][4];

    struct overworld_actor_tile* loaded_actor_tiles[2][2];

    struct { uint16_t x; uint16_t y; } load_next;

    struct hash_map loaded_actors;

    struct overworld_actor actors[MAX_ACTIVE_ACTORS];
    struct overworld_actor* next_free_actor;
    struct overworld_actor* next_active_actor;
};

struct overworld* overworld_load(const char* filename);
void overworld_free(struct overworld* overworld);

#endif