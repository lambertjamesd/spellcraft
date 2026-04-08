#include "line_mesh.h"

#include "../util/hash_map.h"

#define MAX_CONSTRAIN_ITERATIONS  4
#define MAX_EDGE_ITERATIONS  4

static hash_map_t line_mesh_mapping;

void line_mesh_init(line_mesh_t* line_mesh, struct line_mesh_definition* definition, entity_id entity_id) {
    line_mesh->position = definition->position;

    line_mesh->entity_id = entity_id;

    line_mesh->point_count = definition->mesh->point_count;
    line_mesh->line_count = definition->mesh->line_count;

    int points_size = sizeof(vector3_t) * line_mesh->point_count;

    line_mesh->points = malloc(points_size + sizeof(line_mesh_edge_t) * line_mesh->line_count);
    line_mesh->edges = (line_mesh_edge_t*)(line_mesh->points + line_mesh->point_count);

    memcpy(line_mesh->points, definition->mesh->data, points_size);
    memcpy(line_mesh->edges, (char*)definition->mesh->data + points_size, sizeof(line_mesh_edge_t) * line_mesh->line_count);

    hash_map_set(&line_mesh_mapping, entity_id, line_mesh);
}

void line_mesh_destroy(line_mesh_t* line_mesh, struct line_mesh_definition* definition) {
    hash_map_delete(&line_mesh_mapping, line_mesh->entity_id);
    free(line_mesh->points);
    line_mesh->points = NULL;
}

void line_mesh_common_init() {
    hash_map_init(&line_mesh_mapping, 4);
}

void line_mesh_common_destroy() {
    hash_map_destroy(&line_mesh_mapping);
}

line_mesh_t* line_mesh_lookup(entity_id id) {
    return hash_map_get(&line_mesh_mapping, id);
}

uint8_t line_mesh_find_edge_for_vertex(line_mesh_t* mesh, uint8_t point_index) {
    line_mesh_edge_t* end = mesh->edges + mesh->line_count;
    
    for (line_mesh_edge_t* edge = mesh->edges; edge < end; ++edge) {
        if (edge->a == point_index || edge->b == point_index) {
            return edge - mesh->edges;
        }
    }

    return NO_EDGE;
}

uint8_t line_mesh_find_edge_at_vertex(line_mesh_t* mesh, uint8_t vertex_index, uint8_t edge_index, vector3_t* pos) {
    float score = 0.0f;
    uint8_t prev = NO_EDGE;
    uint8_t start_edge = edge_index;
    uint8_t result = NO_EDGE;
    
    vector3_t* from = &mesh->points[vertex_index];
    vector3_t pos_offset;
    vector3Sub(pos, from, &pos_offset);
    
    for (int iter = 0; iter < MAX_EDGE_ITERATIONS && (prev == NO_EDGE || edge_index != start_edge); ++iter) {
        line_mesh_edge_t* edge = &mesh->edges[edge_index];

        vector3_t* to;
        uint8_t to_edge;

        if (edge->a == vertex_index) {
            to = &mesh->points[edge->b];
            to_edge = edge->next_edge_b;
        } else {
            to = &mesh->points[edge->a];
            to_edge = edge->next_edge_a;
        }

        vector3_t edge_offset;
        vector3Sub(to, from, &edge_offset);

        float curr_score = vector3Dot(&edge_offset, &pos_offset);
        
        if (curr_score > score) {
            score = curr_score;
            result = to_edge;
        }

        prev = edge_index;
    }

    return result;
}

uint8_t line_mesh_find_closest_edge(line_mesh_t* mesh, vector3_t* pos) {
    if (!mesh->point_count) {
        return NO_EDGE;
    }

    int closest_vtx = 0;
    float distance = vector3DistSqrd(pos, &mesh->points[0]);

    for (int i = 1; i < mesh->point_count; i += 1) {
        float test = vector3DistSqrd(pos, &mesh->points[i]);

        if (test < distance) {
            closest_vtx = i;
            distance = test;
        }
    }

    return line_mesh_find_edge_for_vertex(mesh, closest_vtx);
}

void line_mesh_constrain_to_mesh(line_mesh_t* mesh, vector3_t* pos, vector3_t* vel, uint8_t* current_edge) {
    uint8_t search_edge = *current_edge;

    if (search_edge == NO_EDGE) {
        search_edge = line_mesh_find_closest_edge(mesh, pos);
    
        if (search_edge == NO_EDGE) {
            return;
        }
    }

    for (int iter = 0; iter < MAX_CONSTRAIN_ITERATIONS; ++iter) {
        line_mesh_edge_t* edge = &mesh->edges[search_edge];

        vector3_t* a = &mesh->points[edge->a];
        vector3_t* b = &mesh->points[edge->b];

        vector3_t offset;
        vector3Sub(a, b, &offset);

        vector3_t pos_offset;
        vector3Sub(pos, b, &pos_offset);

        float offset_inv = 1.0f / vector3MagSqrd(&offset);

        float lerp = vector3Dot(&pos_offset, &offset) * offset_inv;
        vector3Scale(&offset, vel, offset_inv * vector3Dot(vel, &offset));

        if (lerp >= 0.0f && lerp <= 1.0f){
            vector3AddScaled(b, &offset, lerp, pos);
            break;
        }
        
        if (lerp > 1.0f) {
            search_edge = line_mesh_find_edge_at_vertex(mesh, edge->a, search_edge, pos);

            if (search_edge == NO_EDGE) {
                vector3AddScaled(b, &offset, 1.0f, pos);
                *vel = gZeroVec;
                break;
            }
        } else {
            search_edge = line_mesh_find_edge_at_vertex(mesh, edge->b, search_edge, pos);

            if (search_edge == NO_EDGE) {
                vector3AddScaled(b, &offset, 0.0f, pos);
                *vel = gZeroVec;
                break;
            }
        }
    }

    *current_edge = search_edge;
}