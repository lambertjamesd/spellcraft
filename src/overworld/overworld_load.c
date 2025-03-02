#include "overworld_load.h"

#include <malloc.h>

struct overworld_tile* overworld_tile_load(FILE* file) {
    struct overworld_tile* result = malloc(sizeof(struct overworld_tile));
    fread(&result->terrain_mesh_count, 2, 1, file);

    if (result->terrain_mesh_count) {
        result->terrain_meshes = malloc(sizeof(struct tmesh) * result->terrain_mesh_count);
    } else {
        result->terrain_meshes = NULL;
    }

    for (int i = 0; i < result->terrain_mesh_count; i += 1) {
        tmesh_load(&result->terrain_meshes[i], file);
    }

    rspq_block_begin();

    for (int i = 0; i < result->terrain_mesh_count; i += 1) {
        struct tmesh* mesh = &result->terrain_meshes[i];

        rspq_block_run(mesh->material->block);
        rspq_block_run(mesh->block);
    }

    result->render_block = rspq_block_end();

    return result;
}

void overworld_tile_free(struct overworld_tile* tile) {
    for (int i = 0; i < tile->terrain_mesh_count; i += 1) {
        tmesh_release(&tile->terrain_meshes[i]);
    }
    free(tile->terrain_meshes);
    free(tile);
}

struct overworld* overworld_load(const char* filename) {
    struct overworld* result = malloc(sizeof(struct overworld));
    memset(result, 0, sizeof(struct overworld));

    result->file = fopen(filename, "rb");

    fread(&result->tile_x, sizeof(uint16_t), 1, result->file);
    fread(&result->tile_y, sizeof(uint16_t), 1, result->file);
    fread(&result->min, sizeof(struct Vector2), 1, result->file);
    fread(&result->tile_size, sizeof(float), 1, result->file);
    result->inv_tile_size = 1.0f / result->tile_size;
    result->tile_definitions = malloc(result->tile_x * result->tile_y * sizeof(struct overworld_tile_def));
    
    result->load_next.x = NO_TILE_COORD;
    result->load_next.y = NO_TILE_COORD;

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
    overworld->loaded_tiles[slot_x][slot_y] = overworld_tile_load(overworld->file);
    overworld->render_blocks[slot_x][slot_y] = (struct overworld_tile_render_block){
        .render_block = overworld->loaded_tiles[slot_x][slot_y]->render_block,
        .x = overworld->load_next.x,
        .y = overworld->load_next.y,
    };

    overworld->load_next.x = NO_TILE_COORD;
    overworld->load_next.y = NO_TILE_COORD;
}