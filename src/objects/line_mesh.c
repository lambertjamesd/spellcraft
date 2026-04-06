#include "line_mesh.h"

void line_mesh_init(line_mesh_t* line_mesh, struct line_mesh_definition* definition, entity_id entity_id) {
    line_mesh->position = definition->position;

    line_mesh->point_count = definition->mesh->point_count;
    line_mesh->line_count = definition->mesh->line_count;

    int points_size = sizeof(vector3_t) * line_mesh->point_count;

    line_mesh->points = malloc(points_size + sizeof(line_mesh_edge_t) * line_mesh->line_count);
    line_mesh->edges = (line_mesh_edge_t*)(line_mesh->points + line_mesh->point_count);

    memcpy(line_mesh->points, definition->mesh->data, points_size);
    memcpy(line_mesh->edges, (char*)definition->mesh->data + points_size, sizeof(line_mesh_edge_t) * line_mesh->line_count);
}

void line_mesh_destroy(line_mesh_t* line_mesh, struct line_mesh_definition* definition) {
    free(line_mesh->points);
    line_mesh->points = NULL;
}

void line_mesh_common_init() {

}

void line_mesh_common_destroy() {

}