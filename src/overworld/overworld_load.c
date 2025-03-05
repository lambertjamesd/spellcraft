#include "overworld_load.h"

#include <malloc.h>
#include <assert.h>
#include "../render/render_scene.h"
#include "overworld_render.h"

struct overworld_tile* overworld_tile_load(FILE* file) {
    struct overworld_tile* result = malloc(sizeof(struct overworld_tile));
    tmesh_load(&result->terrain_mesh, file);
    fread(&result->starting_y, sizeof(float), 1, file);
    fread(&result->scale_y, sizeof(float), 1, file);
    return result;
}

void overworld_tile_free(struct overworld_tile* tile) {
    tmesh_release(&tile->terrain_mesh);
    free(tile);
}

// EXPR
#define EXPECTED_OVERWORLD_HEADER 0x4F565744

void overworld_render_step(void* data, mat4x4 view_proj_matrix, struct Vector3* camera_position, struct frame_memory_pool* pool) {
    struct overworld* overworld = (struct overworld*)data;
    overworld_render(overworld, view_proj_matrix, camera_position, pool);
}

struct overworld* overworld_load(const char* filename) {
    struct overworld* result = malloc(sizeof(struct overworld));
    memset(result, 0, sizeof(struct overworld));

    result->file = fopen(filename, "rb");

    int header;
    fread(&header, 1, 4, result->file);
    assert(header == EXPECTED_OVERWORLD_HEADER);

    fread(&result->tile_x, sizeof(uint16_t), 1, result->file);
    fread(&result->tile_y, sizeof(uint16_t), 1, result->file);
    fread(&result->min, sizeof(struct Vector2), 1, result->file);
    fread(&result->tile_size, sizeof(float), 1, result->file);
    result->inv_tile_size = 1.0f / result->tile_size;
    result->tile_definitions = malloc(result->tile_x * result->tile_y * sizeof(struct overworld_tile_def));

    fread(result->tile_definitions, sizeof(struct overworld_tile_def), result->tile_x * result->tile_y, result->file);
    
    result->load_next.x = NO_TILE_COORD;
    result->load_next.y = NO_TILE_COORD;

    render_scene_add_step(overworld_render_step, result);

    return result;
}

void overworld_free(struct overworld* overworld) {
    for (int x = 0; x < 4; x += 1) {
        for (int y = 0; y < 4; y += 1) {
            if (overworld->loaded_tiles[x][y]) {
                overworld_tile_free(overworld->loaded_tiles[x][y]);
            }
        }
    }

    render_scene_remove_step(overworld);

    fclose(overworld->file);
    free(overworld->tile_definitions);
    free(overworld);
}

void overworld_check_loaded_tiles(struct overworld* overworld) {
    if (overworld->load_next.x == NO_TILE_COORD || overworld->load_next.y == NO_TILE_COORD) {
        return;
    }

    int slot_x = overworld->load_next.x & 0x3;
    int slot_y = overworld->load_next.y & 0x3;

    if (overworld->loaded_tiles[slot_x][slot_y]) {
        rdpq_call_deferred((void (*)(void*))overworld_tile_free, overworld->loaded_tiles[slot_x][slot_y]);
    }

    fseek(overworld->file, overworld->tile_definitions[overworld->load_next.x + overworld->load_next.y * overworld->tile_x].file_offset, SEEK_SET);
    
    struct overworld_tile* tile = overworld_tile_load(overworld->file);
    overworld->loaded_tiles[slot_x][slot_y] = tile;
    overworld->render_blocks[slot_x][slot_y] = (struct overworld_tile_render_block){
        .render_block = tile->terrain_mesh.block,
        .x = overworld->load_next.x,
        .y = overworld->load_next.y,
        .scale_y = tile->scale_y,
        .starting_y = tile->starting_y,
    };

    overworld->load_next.x = NO_TILE_COORD;
    overworld->load_next.y = NO_TILE_COORD;
}