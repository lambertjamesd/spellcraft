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