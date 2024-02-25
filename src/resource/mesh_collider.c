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
}

void mesh_collider_release(struct mesh_collider* mesh) {
    free(mesh->vertices);
    free(mesh->triangles);
}