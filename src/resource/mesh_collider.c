#include "mesh_collider.h"

#include <malloc.h>
#include <assert.h>

// CMSH
#define EXPECTED_HEADER 0x434D5348

void mesh_collider_load(struct mesh_collider* into, FILE* file) {
    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    uint16_t vertex_count;
    fread(&vertex_count, 2, 1, file);

    into->vertices = malloc(sizeof(struct Vector3) * vertex_count);
    fread(into->vertices, sizeof(struct Vector3), vertex_count, file);

    uint16_t triangle_count;
    fread(&triangle_count, 2, 1, file);

    into->triangles = malloc(sizeof(struct mesh_triangle_indices) * triangle_count);
    fread(into->triangles, sizeof(struct mesh_triangle_indices), triangle_count, file);

    into->triangle_count = triangle_count;

    fread(&into->index.min, sizeof(struct Vector3), 1, file);
    fread(&into->index.stride_inv, sizeof(struct Vector3), 1, file);
    fread(&into->index.block_count, sizeof(struct Vector3u8), 1, file);

    uint16_t index_count;
    fread(&index_count, sizeof(uint16_t), 1, file);

    into->index.index_indices = malloc(sizeof(uint16_t) * index_count);
    fread(into->index.index_indices, sizeof(uint16_t), index_count, file);

    int total_block_count = (int)into->index.block_count.x * (int)into->index.block_count.y * (int)into->index.block_count.z;

    into->index.blocks = malloc(sizeof(struct mesh_index_block) * total_block_count);
    fread(into->index.blocks, sizeof(struct mesh_index_block), total_block_count, file);
}

void mesh_collider_release(struct mesh_collider* mesh) {
    free(mesh->vertices);
    free(mesh->triangles);

    free(mesh->index.index_indices);
    free(mesh->index.blocks);
}